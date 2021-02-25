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

#include "export_configuration.hpp"

class check_on_exception : public std::exception {
    std::string m_msg;

public:
    check_on_exception(const std::string& index, const std::string& value, const std::string& rank)
            : m_msg("invalid data: rank: " + rank + ", at index: " + index +
                    ", Has got: " + value) {}

    virtual const char* what() const throw() {
        return m_msg.c_str();
    }
};

class comparison_exception : public std::exception {
    std::string m_msg;

public:
    comparison_exception(const std::string& rank,
                         const std::string& buffer_idx,
                         const std::string& kernel_val,
                         const std::string& host_val)
            : m_msg("invalid data - rank: " + rank + "; buffer_idx: " + buffer_idx +
                    " -- kernel val: " + kernel_val + "; host_val: " + host_val) {}

    virtual const char* what() const throw() {
        return m_msg.c_str();
    }
};

template <class Container>
typename Container::mapped_type& find_storage_val(Container& storage, int rank) {
    if (storage.find(rank) == storage.end()) {
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                 std::string("cannot find storage for rank: ") +
                                 std::to_string(rank));
    }
    return storage.find(rank)->second;
}

template <class Arr_t, class Type>
ze_result_t zeKernelSetArgumentValueWrap(ze_kernel_handle_t kernel, Arr_t& offset, Type& handle) {
    ze_result_t result = zeKernelSetArgumentValue(kernel, offset, sizeof(handle), &handle);
    if (result != ZE_RESULT_SUCCESS) {
        const void* ptr = reinterpret_cast<const void*>(&handle);
        std::stringstream ss;
        ss << ptr;
        throw std::runtime_error(std::string("cannot zeKernelSetArgumentValue memory at offset: ") +
                                 std::to_string(offset) + ", mem: " + ss.str() +
                                 "\nerror: " + native::to_string(result));
    }
    return result;
}

template <class Arr_t, class Type>
ze_result_t zeKernelSetArgumentValueWrap(ze_kernel_handle_t kernel,
                                         Arr_t& offset,
                                         native::ccl_device::device_memory<Type>* handle) {
    auto ptr = handle->get_ptr();
    ze_result_t result = zeKernelSetArgumentValue(kernel, offset, sizeof(*ptr), ptr);
    if (result != ZE_RESULT_SUCCESS) {
        const void* ptr_v = reinterpret_cast<const void*>(ptr);
        std::stringstream ss;
        ss << ptr_v;
        throw std::runtime_error(
            std::string("cannot zeKernelSetArgumentValue device memory at offset: ") +
            std::to_string(offset) + ", device mem: " + ss.str() +
            "\nerror: " + native::to_string(result));
    }
    return result;
}

template <class Arr_t>
ze_result_t zeKernelSetArgumentValueWrap(
    ze_kernel_handle_t kernel,
    Arr_t& offset,
    std::shared_ptr<native::ccl_device::device_ipc_memory>& handle) {
    auto h = handle->get_ptr();
    ze_result_t result = zeKernelSetArgumentValue(kernel, offset, sizeof(*h), h);
    if (result != ZE_RESULT_SUCCESS) {
        const void* ptr_v = static_cast<const void*>(h);
        std::stringstream ss;
        ss << ptr_v;
        throw std::runtime_error(
            std::string("cannot zeKernelSetArgumentValue IPC memory at offset: ") +
            std::to_string(offset) + ", IPC mem: " + ss.str() +
            "\nerror: " + native::to_string(result));
    }
    return result;
}

template <class Arr, class Container>
void bind_kernel_args(ze_kernel_handle_t kernel,
                      const size_t rank,
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

        zeKernelSetArgumentValueWrap(kernel, offsets[i], handle);
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

ze_command_queue_desc_t select_compute_group_prop(
    size_t device_index,
    const native::ccl_device::queue_group_properties& queue_props,
    const ze_command_queue_desc_t& default_descr) {
    ze_command_queue_desc_t queue_desc = default_descr;

    // find compute ordinal
    queue_desc.ordinal = std::numeric_limits<uint32_t>::max();
    for (uint32_t i = 0; i < queue_props.size(); i++) {
        // Prefer CCS
        if (queue_props[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE &&
            queue_props[i].numQueues > 1) {
            queue_desc.ordinal = i;
        }
    }
    // if CCS not found, look for RCS/CCCS
    if (queue_desc.ordinal == std::numeric_limits<uint32_t>::max()) {
        for (uint32_t i = 0; i < queue_props.size(); i++) {
            if (queue_props[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
                queue_desc.ordinal = i;
            }
        }
    }

    //calculate index
    queue_desc.index = device_index % queue_props[queue_desc.ordinal].numQueues;

    // make async
    queue_desc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    return queue_desc;
}

template <class T>
struct handles_storage {
    using mem_handles_container = std::list<T*>;
    using rank_handles_container = std::map<size_t, mem_handles_container>;

    std::vector<native::ccl_device::device_memory<T>> allocated_storage;
    rank_handles_container per_rank_storage;

    handles_storage(size_t expected_size = 0) {
        allocated_storage.reserve(expected_size);
    }

    // TODO check on depreaction - perhaps not needed anymore
    void rotate_shared_data(size_t rank, size_t comm_size, size_t group_size) {
        size_t initial_index = 0;
        mem_handles_container& right_mem_handles = per_rank_storage[rank];

        // make to rank elemenst become first in array
        // needed for kernel arguments binding: resident parameters come at fist position
        auto rotation_it = right_mem_handles.begin();
        std::advance(rotation_it, (rank - initial_index) * group_size);
        std::rotate(right_mem_handles.begin(), rotation_it, right_mem_handles.end());
    }

    template <class... Handles>
    void register_shared_data(size_t rank, size_t comm_size, Handles&&... h) {
        std::array<native::ccl_device::device_memory<T>, sizeof...(h)> list{ std::move(h)... };
        std::vector<T*> weak_handles;
        for (auto& h : list) {
            weak_handles.push_back(h.handle);
        }

        std::move(list.begin(), list.end(), std::back_inserter(allocated_storage));

        //to front of left: my rank handles belong to current device
        // -> owns handles has position in beginning of kernel arguments
        mem_handles_container& left_mem_handles = per_rank_storage[rank];
        left_mem_handles.insert(left_mem_handles.end(), weak_handles.begin(), weak_handles.end());

        //to the end of right: next rank handles belong to next device
        // -> foreign handles has position in ending of kernel arguments
        size_t initial_index = rank;
        size_t right_rank = (initial_index + 1) % comm_size;
        do {
            mem_handles_container& right_mem_handles = per_rank_storage[right_rank];
            right_mem_handles.insert(
                right_mem_handles.end(), weak_handles.begin(), weak_handles.end());

            rank++;
            right_rank = (rank + 1) % comm_size;
        } while (right_rank != initial_index);
    }

    void dump_for_rank(std::ostream& out, size_t rank, bool handles_only = false) {
        auto it = per_rank_storage.find(rank);
        if (it == per_rank_storage.end()) {
            std::cerr << __FUNCTION__ << ": invalid rank: " << rank << std::endl;
            abort();
        }
        const auto& handles = it->second;

        out << "rank: " << it->first << std::endl;
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
                    std::cerr << __FUNCTION__ << ": cannot find mem handle" << std::endl;
                    abort();
                }
                std::vector<T> tmp = it->enqueue_read_sync();
                std::copy(tmp.begin(), tmp.end(), std::ostream_iterator<T>(out, ","));
                out << "\n\n" << std::endl;
            }
        }
    }

    void dump(std::ostream& out, bool handles_only = false) {
        for (const auto& val : per_rank_storage) {
            dump_for_rank(out, val.first, handles_only);
        }
    }

    void dump_for_rank_by_index(std::ostream& out,
                                size_t rank,
                                size_t mem_idx,
                                bool handles_only = false) {
        auto it = per_rank_storage.find(rank);
        if (it == per_rank_storage.end()) {
            std::cerr << __FUNCTION__ << ": invalid rank: " << rank << std::endl;
            abort();
        }
        const auto& handles = it->second;

        out << "rank: " << it->first << ", handle: ";
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
                    std::cerr << __FUNCTION__ << ": cannot find mem handle" << std::endl;
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
        for (const auto& val : per_rank_storage) {
            dump_for_rank_by_index(out, val.first, mem_handle_index, handles_only);
        }
    }

    mem_handles_container collect_rank_handles_by_index(size_t rank,
                                                        std::initializer_list<size_t> ids) {
        auto it = per_rank_storage.find(rank);
        if (it == per_rank_storage.end()) {
            std::cerr << __FUNCTION__ << ": invalid rank: " << rank << std::endl;
            abort();
        }

        mem_handles_container ret;
        const auto& handles = it->second;
        size_t list_size = handles.size();
        for (auto it = ids.begin(); it != ids.end(); ++it) {
            if (*it >= list_size) {
                std::cerr << __FUNCTION__ << ": invalid index requested: " << *it
                          << ", max: " << list_size << std::endl;
                abort();
            }
            auto list_it = handles.begin();
            std::advance(list_it, *it);

            ret.push_back(*list_it);
        }
        return ret;
    }

    rank_handles_container collect_handles_by_index(std::initializer_list<size_t> ids) {
        rank_handles_container ret;

        for (const auto& val : per_rank_storage) {
            ret[val.first] = collect_rank_handles_by_index(val.first, ids);
        }
        return ret;
    }

    template <class CheckFunctor, class... Args>
    void check_on(const std::vector<T>& vec,
                  const size_t& rank,
                  CheckFunctor lambda,
                  Args&&... params) {
        using std::to_string;

        auto check_lambda = std::bind(lambda, std::forward<Args>(params)..., std::placeholders::_1);
        auto it = std::find_if_not(vec.begin(), vec.end(), check_lambda);
        if (it != vec.end())
            throw check_on_exception(
                to_string(std::distance(vec.begin(), it)), to_string(*it), to_string(rank));
    }

    template <class CheckFunctor, class... Args>
    void check_results(
        const size_t rank,
        std::ostream& output,
        size_t mem_idx, /* offset into mem_handles: 0 - send buffer, 1 - recv buffer*/
        CheckFunctor funct,
        Args&&... params) {
        auto& mem_handles = per_rank_storage.find(rank)->second;
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
            check_on(tmp, rank, funct, std::forward<Args>(params)...);
        }
    }

    std::vector<T> get_memory(const size_t rank, size_t mem_idx) {
        auto& mem_handles = per_rank_storage.find(rank)->second;
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

            return it->enqueue_read_sync();
        }
        throw std::runtime_error("can not find memory for rank " + std::to_string(rank) +
                                 " and mem_idx " + std::to_string(mem_idx));
    }
};

/*IPC handles*/
template <class T>
struct ipc_server_handles_storage {
    using ipc_shared_handle = std::shared_ptr<native::ccl_device::device_ipc_memory_handle>;
    using ipc_handles_container = std::list<ipc_shared_handle>;

    std::vector<ipc_shared_handle> allocated_storage;
    std::map<size_t, ipc_handles_container> per_rank_storage;

    ipc_server_handles_storage(size_t expected_size = 0) {
        allocated_storage.reserve(expected_size);
    }

    template <class... Handles>
    void create_ipcs(std::shared_ptr<native::ccl_context> ctx,
                     size_t rank,
                     size_t comm_size,
                     Handles*... h) {
        std::array<native::ccl_device::device_memory<T>*, sizeof...(h)> memory_handles{ h... };

        ipc_handles_container& ipc_cont = per_rank_storage[rank];

        std::transform(memory_handles.begin(),
                       memory_handles.end(),
                       std::back_inserter(ipc_cont),
                       [ctx](native::ccl_device::device_memory<T>* memory_handle) {
                           auto device_ptr = memory_handle->get_owner().lock();
                           return device_ptr->create_shared_ipc_memory_handle(memory_handle->handle,
                                                                              ctx);
                       });
    }

    ipc_handles_container& get_handles_container(size_t rank) {
        auto it = per_rank_storage.find(rank);
        if (it == per_rank_storage.end()) {
            std::cerr << __PRETTY_FUNCTION__ << " invalid rank: " << rank << std::endl;
            abort();
        }
        return it->second;
    }

    std::vector<uint8_t> serialize_storage(size_t rank) {
        std::vector<uint8_t> serialized_raw_handles;
        auto it = per_rank_storage.find(rank);
        if (it == per_rank_storage.end()) {
            std::cerr << __PRETTY_FUNCTION__ << " invalid rank: " << rank << std::endl;
            abort();
        }

        //serialize handles
        ipc_handles_container& ipc_cont = it->second;
        size_t offset = 0;
        serialized_raw_handles.push_back((uint8_t)rank); //TODO
        offset += sizeof(uint8_t);
        for (auto& ipc_ptr : ipc_cont) {
            try {
                offset += ipc_ptr->serialize(serialized_raw_handles, offset);
            }
            catch (const std::exception& ex) {
                std::cerr << "cannot serialize ip memory handle: "
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

    std::map<size_t, ipc_client_handles_container> per_rank_storage;

    size_t deserialize(std::shared_ptr<native::ccl_context> ctx,
                       const std::vector<uint8_t>& received_raw_handles,
                       size_t expected_handles,
                       native::ccl_device_platform& global_platform,
                       std::shared_ptr<native::ccl_device_platform> pl) {
        using namespace native;

        size_t restored_handles = 0;
        uint8_t rank = received_raw_handles.front();

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
                    throw std::runtime_error("owner device for received IPC hanlde doesn't exist");
                }
                auto ipc_device_it = std::find_if(
                    global_driver.devices.begin(),
                    global_driver.devices.end(),
                    [owner_device](
                        typename ccl_device_driver::devices_storage_type::value_type& dev) {
                        return dev.second->handle == owner_device->handle;
                    });
                if (ipc_device_it == global_driver.devices.end()) {
                    throw std::runtime_error("cannot find ipc device in global driver");
                }

                per_rank_storage[rank].emplace_back(
                    owner_device->restore_shared_ipc_memory(std::move(recv_ipc_handle), ctx));
                restored_handles++;
            }
            catch (const std::exception& ex) {
                std::stringstream ss;
                std::copy(recv_data_start,
                          recv_data_start + recv_data_size,
                          std::ostream_iterator<int>(ss, ","));
                std::cerr << "cannot deserialize ipc memory handle at: " << ss.str()
                          << "\n, offset: " << recv_data_start - received_raw_handles.data()
                          << ", error: " << ex.what() << std::endl;
                throw std::runtime_error(ex.what());
            }
        }
        return restored_handles;
    }
};
