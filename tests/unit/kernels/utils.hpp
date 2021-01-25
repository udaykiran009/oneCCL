#pragma once

#include <algorithm>
#include <array>
#include <exception>
#include <fstream>
#include <future>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <memory>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define private public
#define protected public
#define MULTI_GPU_SUPPORT
#include "oneapi/ccl/native_device_api/export_api.hpp"
#include "oneapi/ccl/native_device_api/l0/declarations.hpp"
#include "lp.hpp"

#undef protected
#undef private

#ifdef STANDALONE_UT
#define UT_ASSERT(cond, ...) \
    do { \
        if (!(cond)) { \
            std::cerr << __VA_ARGS__ << std::endl; \
            this->set_error(__PRETTY_FUNCTION__); \
            dump(); \
            abort(); \
        } \
    } while (0);
#else
#define UT_ASSERT(cond, ...) \
    do { \
        if (!(cond)) { \
            this->set_error(__PRETTY_FUNCTION__); \
        } \
        { ASSERT_TRUE((cond)) << __VA_ARGS__ << std::endl; } \
    } while (0);
#endif

class check_on_exception : public std::exception {
    std::string m_msg;

public:
    check_on_exception(const std::string& index,
                       const std::string& value,
                       const std::string& thread_idx)
            : m_msg("Invalid data: Thread idx: " + thread_idx + ", At index: " + index +
                    ", Has got: " + value) {}

    virtual const char* what() const throw() {
        return m_msg.c_str();
    }
};

class comparison_exception : public std::exception {
    std::string m_msg;

public:
    comparison_exception(const std::string& thread_idx,
                         const std::string& buffer_idx,
                         const std::string& kernel_val,
                         const std::string& host_val)
            : m_msg("Invalid data - thread_idx: " + thread_idx + "; buffer_idx: " + buffer_idx +
                    " -- kernel val: " + kernel_val + "; host_val: " + host_val) {}

    virtual const char* what() const throw() {
        return m_msg.c_str();
    }
};

template <typename T>
inline void str_to_array(const char* input, std::vector<T>& output, char delimiter) {
    if (!input) {
        return;
    }
    std::stringstream ss(input);
    T temp{};
    while (ss >> temp) {
        output.push_back(temp);
        if (ss.peek() == delimiter) {
            ss.ignore();
        }
    }
}

template <>
inline void str_to_array(const char* input, std::vector<std::string>& output, char delimiter) {
    std::string processes_input(input);

    processes_input.erase(std::remove_if(processes_input.begin(),
                                         processes_input.end(),
                                         [](unsigned char x) {
                                             return std::isspace(x);
                                         }),
                          processes_input.end());

    std::replace(processes_input.begin(), processes_input.end(), delimiter, ' ');
    std::stringstream ss(processes_input);

    while (ss >> processes_input) {
        output.push_back(processes_input);
    }
}

inline int readFromSocket(int socket, unsigned char* buffer, size_t size) {
    size_t bytesRead = 0;
    int result;
    while (bytesRead < size) {
        result = static_cast<int>(read(socket, buffer + bytesRead, size - bytesRead));
        if (result < 0) {
            return -1;
        }

        bytesRead += static_cast<int>(result);
    }
    return 0;
}

inline int writeToSocket(int socket, unsigned char* buffer, size_t size) {
    size_t bytesWritten = 0;
    int result;
    while (bytesWritten < size) {
        result = static_cast<int>(write(socket, buffer + bytesWritten, size - bytesWritten));
        if (result < 0) {
            return -1;
        }

        bytesWritten += static_cast<int>(result);
    }
    return 0;
}

inline std::vector<uint8_t> load_binary_file(const std::string& source_path) {
    std::ifstream stream(source_path, std::ios::in | std::ios::binary);

    std::vector<uint8_t> binary_file;
    if (!stream.good()) {
        std::string error("Failed to load binary file: ");
        error += source_path;
        throw std::runtime_error(error);
    }

    size_t length = 0;
    stream.seekg(0, stream.end);
    length = static_cast<size_t>(stream.tellg());
    stream.seekg(0, stream.beg);

    binary_file.resize(length);
    stream.read(reinterpret_cast<char*>(binary_file.data()), length);
    return binary_file;
}

template <class Container>
typename Container::mapped_type& find_storage_val(Container& storage, int thread_idx) {
    if (storage.find(thread_idx) == storage.end()) {
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                 std::string("Cannot find storage for thread: ") +
                                 std::to_string(thread_idx));
    }
    return storage.find(thread_idx)->second;
}

template <class Arr_t, class Type>
ze_result_t zeKernelSetArgumentValueWrap(ze_kernel_handle_t kernel, Arr_t& offset, Type& handle) {
    return zeKernelSetArgumentValue(kernel, offset, sizeof(handle), &handle);
}

template <class Arr_t, class Type>
ze_result_t zeKernelSetArgumentValueWrap(ze_kernel_handle_t kernel,
                                         Arr_t& offset,
                                         native::ccl_device::device_memory<Type>* handle) {
    auto ptr = handle->get_ptr();
    return zeKernelSetArgumentValue(kernel, offset, sizeof(*ptr), ptr);
}

template <class Arr, class Container>
void bind_kernel_args(ze_kernel_handle_t kernel,
                      const size_t thread_idx,
                      Arr& offsets,
                      Container& handles) {
    size_t i = 0;

    for (auto& handle : handles) {
        if (i >= offsets.size()) {
            break; //only own+right is needed
        }
        if (offsets[i] == -1) {
            i++;
            continue; //skip this argument
        }

        ze_result_t result = zeKernelSetArgumentValueWrap(kernel, offsets[i], handle);
        if (result != ZE_RESULT_SUCCESS) {
            throw std::runtime_error(
                std::string("Cannot zeKernelSetArgumentValue memory at mem_offset: ") +
                std::to_string(offsets[i]) + " index\nError: " + native::to_string(result));
        }
        i++;
    }
}

void queue_sync_processing(native::ccl_device::device_cmd_list& list,
                           native::ccl_device::device_queue& queue) {
    ze_result_t ret = ZE_RESULT_SUCCESS;

    ret = zeCommandListClose(list.handle);
    if (ret != ZE_RESULT_SUCCESS) {
        throw std::runtime_error(std::string("cannot zeCommandListClose, error: ") +
                                 std::to_string(ret));
    }

    ret = zeCommandQueueExecuteCommandLists(queue.handle, 1, &list.handle, nullptr);
    if (ret != ZE_RESULT_SUCCESS) {
        throw std::runtime_error(std::string("cannot zeCommandQueueExecuteCommandLists, error: ") +
                                 std::to_string(ret));
    }

    ret = zeCommandQueueSynchronize(queue.handle, std::numeric_limits<uint32_t>::max());
    if (ret != ZE_RESULT_SUCCESS) {
        throw std::runtime_error(std::string("cannot zeCommandQueueSynchronize, error: ") +
                                 std::to_string(ret));
    }
}

template <class T>
struct handles_storage {
    using mem_handles_container = std::list<T*>;
    using thread_handles_container = std::map<size_t, mem_handles_container>;

    std::vector<native::ccl_device::device_memory<T>> allocated_storage;
    thread_handles_container per_thread_storage;

    handles_storage(size_t expected_size) {
        allocated_storage.reserve(expected_size);
    }

    // TODO check on depreaction - perhaps not needed anymore
    void rotate_shared_data(size_t thread_id, size_t threads_count, size_t group_size) {
        size_t initial_index = 0;
        mem_handles_container& right_mem_handles = per_thread_storage[thread_id];

        // make to rank (thread_id or device_id) elemenst become first in array
        //Needed to kernel arguments binding: Resident Parameters come at fist position
        auto rotation_it = right_mem_handles.begin();
        std::advance(rotation_it, (thread_id - initial_index) * group_size);
        std::rotate(right_mem_handles.begin(), rotation_it, right_mem_handles.end());
    }

    template <class... Handles>
    void register_shared_data(size_t thread_idx, size_t threads_count, Handles&&... h) {
        std::array<native::ccl_device::device_memory<T>, sizeof...(h)> list{ std::move(h)... };
        std::vector<T*> weak_handles;
        for (auto& h : list) {
            weak_handles.push_back(h.handle);
        }

        std::move(list.begin(), list.end(), std::back_inserter(allocated_storage));

        //to front of left: my thread handles belong to current device
        // -> owns handles has position in beginning of kernel arguments
        mem_handles_container& left_mem_handles = per_thread_storage[thread_idx];
        left_mem_handles.insert(left_mem_handles.end(), weak_handles.begin(), weak_handles.end());

        //to the end of right: next thread id handles belong to next device
        // -> foreign handles has position in ending of kernel arguments
        size_t initial_index = thread_idx;
        size_t right_thread_idx = (initial_index + 1) % threads_count;
        do {
            mem_handles_container& right_mem_handles = per_thread_storage[right_thread_idx];
            right_mem_handles.insert(
                right_mem_handles.end(), weak_handles.begin(), weak_handles.end());

            thread_idx++;
            right_thread_idx = (thread_idx + 1) % threads_count;
        } while (right_thread_idx != initial_index);
    }

    void dump_for_thread(std::ostream& out, size_t thread_idx, bool handles_only = false) {
        auto it = per_thread_storage.find(thread_idx);
        if (it == per_thread_storage.end()) {
            std::cerr << __FUNCTION__ << ": Invalid thread_idx: " << thread_idx << std::endl;
            abort();
        }
        const auto& handles = it->second;

        out << "Thread: " << it->first << std::endl;
        for (const auto& mem : handles) {
            out << (void*)mem << ", ";
        }
        out << std::endl;

        if (!handles_only) {
            for (const auto& mem : handles) {
                const T* data = mem;
                auto it = std::find_if(allocated_storage.begin(),
                                       allocated_storage.end(),
                                       [data](const native::ccl_device::device_memory<T>& wrapper) {
                                           return wrapper.handle == data;
                                       });

                if (it == allocated_storage.end()) {
                    std::cerr << __FUNCTION__ << ": Cannot find mem handle" << std::endl;
                    abort();
                }
                std::vector<T> tmp = it->enqueue_read_sync();
                std::copy(tmp.begin(), tmp.end(), std::ostream_iterator<T>(out, ","));
                out << "\n\n" << std::endl;
            }
        }
    }

    void dump(std::ostream& out, bool handles_only = false) {
        for (const auto& val : per_thread_storage) {
            dump_for_thread(out, val.first, handles_only);
        }
    }

    void dump_for_thread_by_index(std::ostream& out,
                                  size_t thread_idx,
                                  size_t mem_idx,
                                  bool handles_only = false) {
        auto it = per_thread_storage.find(thread_idx);
        if (it == per_thread_storage.end()) {
            std::cerr << __FUNCTION__ << ": Invalid thread_idx: " << thread_idx << std::endl;
            abort();
        }
        const auto& handles = it->second;

        out << "Thread: " << it->first << ", handle: ";
        size_t i = 0;
        for (auto it = handles.begin(); it != handles.end(); it++, i++) {
            if (i != mem_idx) {
                continue;
            }
            auto* mem = *it;
            out << (void*)mem << std::endl;

            if (!handles_only) {
                const T* data = mem;
                auto it = std::find_if(allocated_storage.begin(),
                                       allocated_storage.end(),
                                       [data](const native::ccl_device::device_memory<T>& wrapper) {
                                           return wrapper.handle == data;
                                       });

                if (it == allocated_storage.end()) {
                    std::cerr << __FUNCTION__ << ": Cannot find mem handle" << std::endl;
                    abort();
                }
                //size_t buffer_size = it->count();
                std::vector<T> tmp = it->enqueue_read_sync();
                for (size_t i = 0; i < tmp.size() - 1; ++i) {
                    out << i << ": " << tmp[i] << "; ";
                }
                out << tmp.size() - 1 << ": " << tmp[tmp.size() - 1] << ";" << std::endl;
                out << "\n\n" << std::endl;
            }
        }
    }
    void dump_by_index(std::ostream& out, size_t mem_handle_index, bool handles_only = false) {
        for (const auto& val : per_thread_storage) {
            dump_for_thread_by_index(out, val.first, mem_handle_index, handles_only);
        }
    }

    mem_handles_container collect_thread_handles_by_index(size_t thread_idx,
                                                          std::initializer_list<size_t> ids) {
        auto it = per_thread_storage.find(thread_idx);
        if (it == per_thread_storage.end()) {
            std::cerr << __FUNCTION__ << ": Invalid thread_idx: " << thread_idx << std::endl;
            abort();
        }

        mem_handles_container ret;
        const auto& handles = it->second;
        size_t list_size = handles.size();
        for (auto it = ids.begin(); it != ids.end(); ++it) {
            if (*it >= list_size) {
                std::cerr << __FUNCTION__ << ": Invalid index requested: " << *it
                          << ", max: " << list_size << std::endl;
                abort();
            }
            auto list_it = handles.begin();
            std::advance(list_it, *it);

            ret.push_back(*list_it);
        }
        return ret;
    }

    thread_handles_container collect_handles_by_index(std::initializer_list<size_t> ids) {
        thread_handles_container ret;

        for (const auto& val : per_thread_storage) {
            ret[val.first] = collect_thread_handles_by_index(val.first, ids);
        }
        return ret;
    }

    template <class CheckFunctor, class... Args>
    void check_on(const std::vector<T>& vec,
                  const size_t& thread_idx,
                  CheckFunctor lambda,
                  Args&&... params) {
        using std::to_string;

        auto check_lambda = std::bind(lambda, std::forward<Args>(params)..., std::placeholders::_1);
        auto it = std::find_if_not(vec.begin(), vec.end(), check_lambda);
        if (it != vec.end())
            throw check_on_exception(
                to_string(std::distance(vec.begin(), it)), to_string(*it), to_string(thread_idx));
    }

    template <class CheckFunctor, class... Args>
    void check_results(
        const size_t& thread_idx,
        std::ostream& output,
        size_t mem_idx, /* offset into mem_handles: 0 - send buffer, 1 - recv buffer*/
        CheckFunctor funct,
        Args&&... params) {
        auto& mem_handles = per_thread_storage.find(thread_idx)->second;
        size_t i = 0;

        for (auto iter_mem = mem_handles.begin(); iter_mem != mem_handles.end(); iter_mem++, i++) {
            if (i != mem_idx)
                continue;

            const auto* data = *iter_mem;
            auto it = std::find_if(allocated_storage.begin(),
                                   allocated_storage.end(),
                                   [data](const native::ccl_device::device_memory<T>& wrapper) {
                                       return wrapper.handle == data;
                                   });

            std::vector<T> tmp = it->enqueue_read_sync();
            check_on(tmp, thread_idx, funct, std::forward<Args>(params)...);
        }
    }
};

/*IPC handles*/
template <class T>
struct ipc_server_handles_storage {
    using ipc_shared_handle = std::shared_ptr<native::ccl_device::device_ipc_memory_handle>;
    using ipc_handles_container = std::list<ipc_shared_handle>;

    std::vector<ipc_shared_handle> allocated_storage;
    std::map<size_t, ipc_handles_container> per_thread_storage;

    ipc_server_handles_storage(size_t expected_size) {
        allocated_storage.reserve(expected_size);
    }

    template <class... Handles>
    void create_ipcs(std::shared_ptr<native::ccl_context> ctx,
                     size_t thread_idx,
                     size_t threads_count,
                     Handles*... h) {
        std::array<native::ccl_device::device_memory<T>*, sizeof...(h)> memory_handles{ h... };

        ipc_handles_container& ipc_cont = per_thread_storage[thread_idx];

        std::transform(memory_handles.begin(),
                       memory_handles.end(),
                       std::back_inserter(ipc_cont),
                       [ctx](native::ccl_device::device_memory<T>* memory_handle) {
                           auto device_ptr = memory_handle->get_owner().lock();
                           return device_ptr->create_shared_ipc_memory_handle(memory_handle->handle,
                                                                              ctx);
                       });
    }

    std::vector<uint8_t> serialize_storage(size_t thread_idx) {
        std::vector<uint8_t> serialized_raw_handles;
        auto it = per_thread_storage.find(thread_idx);
        if (it == per_thread_storage.end()) {
            std::cerr << __PRETTY_FUNCTION__ << " invalid thred id: " << thread_idx << std::endl;
            abort();
        }

        //serialize handles
        ipc_handles_container& ipc_cont = it->second;
        size_t offset = 0;
        serialized_raw_handles.push_back((uint8_t)thread_idx); //TODO
        offset += sizeof(uint8_t);
        for (auto& ipc_ptr : ipc_cont) {
            try {
                offset += ipc_ptr->serialize(serialized_raw_handles, offset);
            }
            catch (const std::exception& ex) {
                std::cerr << "Cannot serialize ip memory handle: "
                          << native::to_string(ipc_ptr->handle) << "\nError: " << ex.what()
                          << std::endl;
                abort();
            }
        }
        return serialized_raw_handles;
    }
};

template <class T>
struct ipc_client_handles_storage {
    using ipc_client_shared = std::shared_ptr<native::ccl_device::device_ipc_memory>;
    using ipc_client_handles_container = std::list<ipc_client_shared>;

    std::map<size_t, ipc_client_handles_container> per_thread_storage;

    size_t deserialize(std::shared_ptr<native::ccl_context> ctx,
                       const std::vector<uint8_t>& received_raw_handles,
                       size_t expected_handles,
                       native::ccl_device_platform& global_platform,
                       std::shared_ptr<native::ccl_device_platform> pl) {
        using namespace native;

        size_t restored_handles = 0;
        uint8_t thread_id = received_raw_handles.front();

        const uint8_t* recv_data_start = received_raw_handles.data() + sizeof(uint8_t);
        size_t recv_data_size = received_raw_handles.size() - sizeof(uint8_t);

        ccl_device_driver& global_driver = *global_platform.drivers.find(0)->second;

        while (recv_data_size > 0 and restored_handles < expected_handles) {
            try {
                auto recv_ipc_handle = ccl_device::device_ipc_memory_handle::deserialize<
                    ccl_device::device_ipc_memory_handle>(
                    &recv_data_start, recv_data_size, ctx, pl);

                std::shared_ptr<ccl_device> owner_device = recv_ipc_handle->get_owner().lock();
                if (!owner_device) {
                    throw std::runtime_error("Owner device for received IPC hanlde doesn't exist");
                }
                auto ipc_device_it = std::find_if(
                    global_driver.devices.begin(),
                    global_driver.devices.end(),
                    [owner_device](
                        typename ccl_device_driver::devices_storage_type::value_type& dev) {
                        return dev.second->handle == owner_device->handle;
                    });
                if (ipc_device_it == global_driver.devices.end()) {
                    throw std::runtime_error("Cannot find ipc device in global driver");
                }

                per_thread_storage[thread_id].emplace_back(
                    owner_device->restore_shared_ipc_memory(std::move(recv_ipc_handle), ctx));
                restored_handles++;
            }
            catch (const std::exception& ex) {
                std::stringstream ss;
                std::copy(recv_data_start,
                          recv_data_start + recv_data_size,
                          std::ostream_iterator<int>(ss, ","));
                std::cerr << "Cannot deserialize ipc memory handle at: " << ss.str()
                          << "\n. Offset: " << recv_data_start - received_raw_handles.data()
                          << ". Error: " << ex.what() << std::endl;
                throw std::runtime_error(ex.what());
            }
        }
        return restored_handles;
    }
};
