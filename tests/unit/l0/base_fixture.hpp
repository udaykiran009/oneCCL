#pragma once
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>

#include "utils.hpp"
#include "ccl.hpp"
//#define UT_DEBUG

#ifdef UT_DEBUG
    std::ostream& global_output = std::cout;
#else
    std::stringstream global_ss;
    std::ostream& global_output = global_ss;
#endif

struct tracer
{
    tracer():
     output(global_output)
    {
        is_error = false;
    }

    ~tracer()
    {
        if(is_error)
        {
            dump();
        }
    }

    void set_error(const std::string& message) const
    {
        error_str =  std::string("\n") + message +"\n";
        is_error = true;
    }

    void dump() const
    {
        std::stringstream ss;
        ss << output.rdbuf();
        std::cerr << "\n/**********************************************"
                     "Test Error***************************************\n"
                     "Test Case Last Error:\t" << error_str << "Full Log:\n" << ss.str() << std::endl;
    }

    std::ostream& output;
    mutable std::string error_str;
    mutable bool is_error;
};

class common_fixture : public testing::Test, public tracer
{
protected:
    common_fixture(const std::string& cluster_affinity) :
        tracer()
    {
        // parse affinity
        std::vector<std::string> gpu_affinity_array;
        str_to_array<std::string>(cluster_affinity.c_str(), gpu_affinity_array, ',');

        // convert to bitset
        global_affinities.reserve(gpu_affinity_array.size());
        std::transform(gpu_affinity_array.begin(), gpu_affinity_array.end(),
                       std::back_inserter(global_affinities), [](const std::string& mask)
                       {
                           return ccl::device_indices_t { ccl::from_string(mask) };
                       });

    }

    virtual ~common_fixture()
    {
    }

    void create_global_platform()
    {
        // enumerates all available driver and all available devices(for foreign devices ipc handles recover)
        global_platform = native::ccl_device_platform::create();
    }
    void create_local_platform()
    {
        //TODO only driver by 0 idx
        local_platform = native::ccl_device_platform::create(local_affinity);
    }

    void create_module_descr(const std::string& module_path, bool replace = false)
    {
        ze_module_desc_t module_description;
        std::vector<uint8_t> binary_module_file;
        try
        {
            binary_module_file = load_binary_file(module_path);
            module_description.version = ZE_MODULE_DESC_VERSION_CURRENT;
            module_description.format = ZE_MODULE_FORMAT_IL_SPIRV;
            module_description.inputSize = static_cast<uint32_t>(binary_module_file.size());
            module_description.pInputModule = binary_module_file.data();
            module_description.pBuildFlags = nullptr;
        }
        catch (const std::exception& ex)
        {
            UT_ASSERT(false, "Cannot load kernel: " << module_path << "\nError: " << ex.what());
            throw;
        }

        // compile test module per device

        auto it = local_platform->drivers.find(0);
        UT_ASSERT(it != local_platform->drivers.end(), "Driver idx 0 must exist");
        for(auto &dev_it : it->second->devices)
        {
            auto dev = dev_it.second;
            try
            {
                if(!replace)
                {
                    device_modules.emplace(dev.get(),
                                           dev->create_module(module_description));
                }
                else
                {
                    device_modules.erase(dev.get());
                    device_modules.emplace(dev.get(),
                                           dev->create_module(module_description));

                }
            }
            catch(const std::exception& ex)
            {
                UT_ASSERT(false, "Cannot load module on device: " << dev->to_string() << "\nError: " << ex.what());
            }

            //create for subdevices
            auto& subdevices = dev->get_subdevices();
            for(auto &subdev : subdevices)
            {
                try
                {
                    if(!replace)
                    {
                        device_modules.emplace(subdev.second.get(),
                                               subdev.second->create_module(module_description));
                    }
                    else
                    {
                        device_modules.erase(subdev.second.get());
                        device_modules.emplace(subdev.second.get(),
                                            subdev.second->create_module(module_description));

                    }
                }
                catch(const std::exception& ex)
                {
                    UT_ASSERT(false, "Cannot load module on SUB device: " << subdev.second->to_string() << "\nError: " << ex.what());
                }
            }
        }
    }


    std::shared_ptr<native::ccl_device_platform> global_platform;
    std::shared_ptr<native::ccl_device_platform> local_platform;
    std::vector<ccl::device_indices_t> global_affinities;
    ccl::device_indices_t local_affinity;


    std::map<native::ccl_device*, native::ccl_device::device_module> device_modules;
};

class ipc_fixture : public common_fixture
{
public:
    int  wait_for_child_fork(int pid)
    {
        return 0;
    }

protected:
    ipc_fixture(const std::string& node_affinity = std::string("[0:0],[0:1]"),
                const std::string& module = std::string("kernels/ipc_test.spv")) :
        common_fixture(node_affinity),
        module_path(module)
    {
    }

    virtual ~ipc_fixture() override
    {
    }

    bool is_child() const
    {
        return !pid;
    }

    void SetUp() override
    {
        if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sockets) < 0)
        {
            perror("socketpair fails");
            exit(1);
        }

        pid = fork();
        if(pid < 0)
        {
            perror("fork fails");
            exit(1);
        }
        else if(pid == 0) //child
        {
            close(sockets[TO_CHILD]);
            local_affinity = global_affinities[TO_CHILD];

            communication_socket = sockets[TO_PARENT];

            // create local to child process driver & devices
            create_global_platform();
            create_local_platform();
            create_module_descr(module_path);
        }
        else
        {
            close(sockets[TO_PARENT]);
            local_affinity = global_affinities[TO_PARENT];

            communication_socket = sockets[TO_CHILD];

            // create local to parent process driver & devices
            create_global_platform();
            create_local_platform();
            create_module_descr(module_path);
        }
    }

    void TearDown() override
    {
        //TODO waitpid for parent
        if(pid)
        {
            std::cout << "PID: "  << pid << " wait child" << std::endl;
            int status = 0;
            int ret = waitpid(pid, &status, 0);
            UT_ASSERT(ret == pid, "Waitpid return fails");
        }
        else
        {
            std::cout << "PID: "  << pid << " exit" << std::endl;
            exit(0);
        }
    }


    int pid = 0;
    enum
    {
        TO_CHILD,
        TO_PARENT
    };

    int sockets[2];
    int communication_socket = 0;

    std::string module_path;
};
