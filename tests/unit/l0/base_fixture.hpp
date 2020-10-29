#pragma once

#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>

#include "lp.hpp"
#include "utils.hpp"

#ifdef STANDALONE_UT
namespace ccl {
template <class type>
struct native_type_info {
    static constexpr bool is_supported = false;
    static constexpr bool is_class = false;
};
}
#else
#include "oneapi/ccl/ccl_type_traits.hpp"
#endif

using bfloat16 = ccl::bfloat16;

#define DECLARE_KERNEL_TYPE(COLL) \
    template <class T>            \
    struct COLL##_param_traits;

#define DECLARE_OP_KERNEL_TYPE(COLL) \
    template <class T, class Op>     \
    struct COLL##_param_traits;

DECLARE_KERNEL_TYPE(allgatherv)
DECLARE_OP_KERNEL_TYPE(allreduce)
DECLARE_KERNEL_TYPE(alltoallv)
DECLARE_KERNEL_TYPE(bcast)
DECLARE_OP_KERNEL_TYPE(reduce)
DECLARE_OP_KERNEL_TYPE(reduce_scatter)

template <class T>
struct my_add {
    T operator()(const T& lhs, const T& rhs) const {
        return lhs + rhs;
    }

    static constexpr T init() { return 0; }
};

template <class T>
struct my_mult {
    T operator()(const T& lhs, const T& rhs) const {
        return lhs * rhs;
    }

    static constexpr T init() { return 1; }
};

template <class T>
struct my_min {
    T operator()(const T& lhs, const T& rhs) const {
        return std::min(lhs, rhs);
    }

    static constexpr T init() { return std::numeric_limits<T>::max(); }
};

template <class T>
struct my_max {
    T operator()(const T& lhs, const T& rhs) const {
        return std::max(lhs, rhs);
    }

    static constexpr T init() { return std::numeric_limits<T>::min(); }
};

template<>
struct my_add<bfloat16> {
    bfloat16 operator()(const bfloat16& lhs, const bfloat16& rhs) const {
        return fp32_to_bf16(bf16_to_fp32(lhs) + bf16_to_fp32(rhs));
    }

    static constexpr bfloat16 init() { return bfloat16(0); }
};

template<>
struct my_mult<bfloat16> {
    bfloat16 operator()(const bfloat16& lhs, const bfloat16& rhs) const {
        return fp32_to_bf16(bf16_to_fp32(lhs) * bf16_to_fp32(rhs));
    }

    static constexpr bfloat16 init() { return bfloat16(1); }
};

template<>
struct my_min<bfloat16> {
    bfloat16 operator()(const bfloat16& lhs, const bfloat16& rhs) const {
        return fp32_to_bf16(std::min(bf16_to_fp32(lhs), bf16_to_fp32(rhs)));
    }

    static constexpr bfloat16 init() { return bfloat16(0x7f7f); }
};

template<>
struct my_max<bfloat16> {
    bfloat16 operator()(const bfloat16& lhs, const bfloat16& rhs) const {
        return fp32_to_bf16(std::max(bf16_to_fp32(lhs), bf16_to_fp32(rhs)));
    }

    static constexpr bfloat16 init() { return bfloat16(0xff7f); }
};

#define DEFINE_KERNEL_TYPE(NAME, T, COLL) \
    template <> \
    struct COLL##_param_traits<T> { \
        static constexpr const char* kernel_name = #COLL "_execution_" #NAME; \
    };

#define DEFINE_KERNEL_TYPES(COLL)              \
    DEFINE_KERNEL_TYPE(int8, int8_t, COLL)                  \
    DEFINE_KERNEL_TYPE(uint8, uint8_t, COLL)                 \
    DEFINE_KERNEL_TYPE(int16, int16_t, COLL)                 \
    DEFINE_KERNEL_TYPE(uint16, uint16_t, COLL)                \
    DEFINE_KERNEL_TYPE(int32, int32_t, COLL)                 \
    DEFINE_KERNEL_TYPE(uint32, uint32_t, COLL)                \
    DEFINE_KERNEL_TYPE(int64, int64_t, COLL)                 \
    DEFINE_KERNEL_TYPE(uint64, uint64_t, COLL)                \
    DEFINE_KERNEL_TYPE(float32, float, COLL)             \
    DEFINE_KERNEL_TYPE(float64, double, COLL)

#define DEFINE_KERNEL_TYPE_FOR_OP(NAME, T, COLL, OP) \
    template <> \
    struct COLL##_param_traits<T, my_##OP<T>> { \
        static constexpr const char* kernel_name = #COLL "_execution_" #NAME "_" #OP; \
        using op_type = my_##OP<T>; \
    };

#define DEFINE_KERNEL_TYPES_FOR_OP(COLL, OP)              \
    DEFINE_KERNEL_TYPE_FOR_OP(int8, int8_t, COLL, OP)                  \
    DEFINE_KERNEL_TYPE_FOR_OP(uint8, uint8_t, COLL, OP)                 \
    DEFINE_KERNEL_TYPE_FOR_OP(int16, int16_t, COLL, OP)                 \
    DEFINE_KERNEL_TYPE_FOR_OP(uint16, uint16_t, COLL, OP)                \
    DEFINE_KERNEL_TYPE_FOR_OP(int32, int32_t, COLL, OP)                 \
    DEFINE_KERNEL_TYPE_FOR_OP(uint32, uint32_t, COLL, OP)                \
    DEFINE_KERNEL_TYPE_FOR_OP(int64, int64_t, COLL, OP)                 \
    DEFINE_KERNEL_TYPE_FOR_OP(uint64, uint64_t, COLL, OP)                \
    DEFINE_KERNEL_TYPE_FOR_OP(float32, float, COLL, OP)             \
    DEFINE_KERNEL_TYPE_FOR_OP(float64, double, COLL, OP)

#define DEFINE_KERNEL_TYPES_FOR_OP_BF16(COLL, OP)         \
    DEFINE_KERNEL_TYPES_FOR_OP(COLL, OP)                 \
    DEFINE_KERNEL_TYPE_FOR_OP(bfloat16, bfloat16, COLL, OP)

using TestTypes = ::testing::Types<
    int8_t,
    uint8_t,
    int16_t,
    uint16_t,
    int32_t,
    uint32_t,
    int64_t,
    uint64_t,
    float,
    double
>;

#define DEFINE_PAIR(T, Op) \
    std::pair<T, Op>

#define DEFINE_TYPE(T)           \
    DEFINE_PAIR(T, my_add<T>),   \
    DEFINE_PAIR(T, my_mult<T>),  \
    DEFINE_PAIR(T, my_min<T>),   \
    DEFINE_PAIR(T, my_max<T>)

// Note: don't use float with mult op as the rounding error gets
// noticeable quite fast
using TestTypesAndOps = ::testing::Types<
    DEFINE_PAIR(int8_t, my_add<int8_t>),
    /*DEFINE_PAIR(uint8_t, my_mult<uint8_t>), */
    DEFINE_PAIR(int16_t, my_min<int16_t>),
    /*DEFINE_PAIR(uint16_t, my_max<uint16_t>),
    DEFINE_PAIR(int32_t, my_add<int32_t>),*/
    DEFINE_PAIR(uint32_t, my_mult<uint32_t>),
    /*DEFINE_PAIR(int64_t, my_min<int64_t>),
    DEFINE_PAIR(uint64_t, my_max<uint64_t>),*/
    DEFINE_PAIR(float, my_max<float>)/*,
    DEFINE_PAIR(double, my_min<double>)*/
>;

/* BF16 in kernels is supported for allreduce only */
using TestTypesAndOpsAllreduce = ::testing::Types<
    /*DEFINE_PAIR(int8_t, my_add<int8_t>),
    DEFINE_PAIR(uint8_t, my_mult<uint8_t>),
    DEFINE_PAIR(int16_t, my_min<int16_t>),
    DEFINE_PAIR(uint16_t, my_max<uint16_t>),
    DEFINE_PAIR(int32_t, my_add<int32_t>),
    DEFINE_PAIR(uint32_t, my_mult<uint32_t>),
    DEFINE_PAIR(int64_t, my_min<int64_t>),
    DEFINE_PAIR(uint64_t, my_max<uint64_t>),
    DEFINE_PAIR(float, my_mult<float>),
    DEFINE_PAIR(double, my_min<double>),*/
    DEFINE_PAIR(bfloat16, my_add<bfloat16>),
    DEFINE_PAIR(bfloat16, my_max<bfloat16>)
>;

#define UT_DEBUG

#ifdef UT_DEBUG
std::ostream& global_output = std::cout;
#else
std::stringstream global_ss;
std::ostream& global_output = global_ss;
#endif

static std::string device_indices{ "[0:6459]" };

void set_test_device_indices(const char* indices_csv) {
    if (indices_csv) {
        device_indices = indices_csv;
    }
}

const std::string& get_global_device_indices() {
    return device_indices;
}

struct tracer {
    tracer() : output(global_output) {
        is_error = false;
    }

    ~tracer() {
        if (is_error) {
            dump();
        }
    }

    void set_error(const std::string& message) const {
        error_str = std::string("\n") + message + "\n";
        is_error = true;
    }

    void dump() const {
        std::stringstream ss;
        ss << output.rdbuf();
        std::cerr << "\n/**********************************************"
                     "Test Error***************************************\n"
                     "Test Case Last Error:\t"
                  << error_str << "Full Log:\n"
                  << ss.str() << std::endl;
    }

    std::ostream& output;
    mutable std::string error_str;
    mutable bool is_error;
};

class common_fixture : public testing::Test, public tracer {
protected:
    common_fixture(const std::string& cluster_affinity) : tracer() {
        // parse affinity
        std::vector<std::string> gpu_affinity_array;
        str_to_array<std::string>(cluster_affinity.c_str(), gpu_affinity_array, ',');

        // convert to bitset
        global_affinities.reserve(gpu_affinity_array.size());
        std::transform(gpu_affinity_array.begin(),
                       gpu_affinity_array.end(),
                       std::back_inserter(global_affinities),
                       [](const std::string& mask) {
                           return ccl::device_indices_type{ ccl::from_string(mask) };
                       });
    }

    virtual ~common_fixture() {}

    void create_global_platform() {
        // enumerates all available driver and all available devices(for foreign devices ipc handles recover)
        global_platform = native::ccl_device_platform::create();
    }
    void create_local_platform() {
        //TODO only driver by 0 idx
        local_platform = native::ccl_device_platform::create(local_affinity);
    }

    void create_module_descr(const std::string& module_path, bool replace = false) {
        ze_module_desc_t module_description;
        std::vector<uint8_t> binary_module_file;
        try {
            binary_module_file = load_binary_file(module_path);
            module_description.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
            module_description.pNext = nullptr;
            module_description.format = ZE_MODULE_FORMAT_IL_SPIRV;
            module_description.inputSize = static_cast<uint32_t>(binary_module_file.size());
            module_description.pInputModule = binary_module_file.data();
            module_description.pBuildFlags = nullptr;
            module_description.pConstants = nullptr;
        }
        catch (const std::exception& ex) {
            UT_ASSERT(false, "Cannot load kernel: " << module_path << "\nError: " << ex.what());
            throw;
        }

        // compile test module per device
        std::size_t hash = std::hash<std::string>{}(module_path);
        auto it = local_platform->drivers.find(0);
        UT_ASSERT(it != local_platform->drivers.end(), "Driver idx 0 must exist");
        for (auto& dev_it : it->second->devices) {
            auto dev = dev_it.second;
            try {
                if (!replace) {
                    device_modules.emplace(dev.get(), dev->create_module(module_description, hash, dev->get_default_context()));
                }
                else {
                    device_modules.erase(dev.get());
                    device_modules.emplace(dev.get(), dev->create_module(module_description, hash, dev->get_default_context()));
                }
            }
            catch (const std::exception& ex) {
                UT_ASSERT(false,
                          "Cannot load module on device: " << dev->to_string()
                                                           << "\nError: " << ex.what());
            }

            //create for subdevices
            auto& subdevices = dev->get_subdevices();
            for (auto& subdev : subdevices) {
                try {
                    if (!replace) {
                        device_modules.emplace(
                            subdev.second.get(),
                            subdev.second->create_module(module_description, hash, subdev.second->get_default_context()));
                    }
                    else {
                        device_modules.erase(subdev.second.get());
                        device_modules.emplace(
                            subdev.second.get(),
                            subdev.second->create_module(module_description, hash, subdev.second->get_default_context()));
                    }
                }
                catch (const std::exception& ex) {
                    UT_ASSERT(false,
                              "Cannot load module on SUB device: " << subdev.second->to_string()
                                                                   << "\nError: " << ex.what());
                }
            }
        }
    }

    std::shared_ptr<native::ccl_device_platform> global_platform;
    std::shared_ptr<native::ccl_device_platform> local_platform;
    std::vector<ccl::device_indices_type> global_affinities;
    ccl::device_indices_type local_affinity;

    std::map<native::ccl_device*, native::ccl_device::device_module_ptr> device_modules;
};

class ipc_fixture : public common_fixture {
public:
    int wait_for_child_fork(int pid) {
        return 0;
    }

protected:
    ipc_fixture(const std::string& node_affinity = device_indices /*"[0:0],[0:1]"*/,
                const std::string& module = std::string("kernels/ipc_test.spv"))
            : common_fixture(node_affinity),
              module_path(module) {}

    virtual ~ipc_fixture() override {}

    bool is_child() const {
        return !pid;
    }

    void SetUp() override {
        if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sockets) < 0) {
            perror("socketpair fails");
            exit(1);
        }

        pid = fork();
        if (pid < 0) {
            perror("fork fails");
            exit(1);
        }
        else if (pid == 0) //child
        {
            close(sockets[TO_CHILD]);
            local_affinity = global_affinities[TO_CHILD];

            communication_socket = sockets[TO_PARENT];

            // create local to child process driver & devices
            create_global_platform();
            create_local_platform();
            create_module_descr(module_path);
        }
        else {
            close(sockets[TO_PARENT]);
            local_affinity = global_affinities[TO_PARENT];

            communication_socket = sockets[TO_CHILD];

            // create local to parent process driver & devices
            create_global_platform();
            create_local_platform();
            create_module_descr(module_path);
        }
    }

    void TearDown() override {
        //TODO waitpid for parent
        if (pid) {
            std::cout << "PID: " << pid << " wait child" << std::endl;
            int status = 0;
            int ret = waitpid(pid, &status, 0);
            UT_ASSERT(ret == pid, "Waitpid return fails");
        }
        else {
            std::cout << "PID: " << pid << " exit" << std::endl;
            exit(0);
        }
    }

    int pid = 0;
    enum { TO_CHILD, TO_PARENT };

    int sockets[2];
    int communication_socket = 0;

    std::string module_path;
};
