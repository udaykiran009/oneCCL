#pragma once

#include <map>
#include <vector>

#include "kernel_utils.hpp"

template <class DType>
class data_storage {
public:
    using native_type = DType;
    enum { data_count_max_limit = 100 };

    void initialize_data_storage(size_t comm_size) {
        this->comm_size = comm_size;
        memory_storage = handles_storage<native_type>(data_count_max_limit * comm_size);
        flags_storage = handles_storage<int>(data_count_max_limit * comm_size);
    }

    // Creators
    template <class... Handles>
    void register_shared_memories_data(size_t rank, Handles&&... h) {
        if (rank >= comm_size) {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                     " - invalid rank requested: " + std::to_string(rank) +
                                     ", comm_size: " + std::to_string(comm_size));
        }
        memory_storage.register_shared_data(rank, comm_size, std::forward<Handles>(h)...);
    }

    template <class... Handles>
    void register_shared_flags_data(size_t rank, Handles&&... h) {
        if (rank >= comm_size) {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                     " - invalid rank requested: " + std::to_string(rank) +
                                     ", comm_size: " + std::to_string(comm_size));
        }
        flags_storage.register_shared_data(rank, comm_size, std::forward<Handles>(h)...);
    }

    template <class... Handles>
    void register_shared_comm_data(size_t rank, Handles&&... h) {
        if (rank >= comm_size) {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                     " - invalid rank requested: " + std::to_string(rank) +
                                     ", comm_size: " + std::to_string(comm_size));
        }

        std::array<int, sizeof...(h)> list{ static_cast<int>(h)... };
        comm_param_storage[rank].insert(comm_param_storage[rank].end(), list.begin(), list.end());
    }

    void finalize_data_registration(size_t comm_group_size,
                                    size_t mem_group_size,
                                    size_t flag_group_size) {
        for (size_t rank = 0; rank < comm_size; rank++) {
            (void)comm_group_size;
            memory_storage.rotate_shared_data(rank, comm_size, mem_group_size);
            flags_storage.rotate_shared_data(rank, comm_size, flag_group_size);
        }
    }

    // Getters
    size_t get_comm_size() const {
        return comm_size;
    }

    typename handles_storage<native_type>::mem_handles_container& get_memory_handles(size_t rank) {
        if (rank >= comm_size) {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                     " - invalid rank requested: " + std::to_string(rank) +
                                     ", comm_size: " + std::to_string(comm_size));
        }
        return find_storage_val(memory_storage.per_rank_storage, rank);
    }

    handles_storage<native_type>& get_memory_storage() {
        return memory_storage;
    }

    typename handles_storage<int>::mem_handles_container& get_flag_handles(size_t rank) {
        if (rank >= comm_size) {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                     " - invalid rank requested: " + std::to_string(rank) +
                                     ", comm_size: " + std::to_string(comm_size));
        }
        return find_storage_val(flags_storage.per_rank_storage, rank);
    }

    std::vector<int>& get_comm_handles(size_t rank) {
        if (rank >= comm_size) {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                     " - invalid rank requested: " + std::to_string(rank) +
                                     ", comm_size: " + std::to_string(comm_size));
        }
        return find_storage_val(comm_param_storage, rank);
    }

    // Utils
    void dump_memory(std::ostream& out, bool handles_only = false) {
        //memory_storage.dump(out, handles_only);
    }

    void dump_memory_by_index(std::ostream& out, size_t mem_handle_index) {
        memory_storage.dump_by_index(out, mem_handle_index);
    }

    // Checking test results
    template <class CheckFunctor, class... Args>
    void check_test_results(
        const size_t& rank,
        std::ostream& output,
        size_t mem_idx, /* offset into mem_handles: 0 - send buffer, 1 - recv buffer*/
        CheckFunctor funct,
        Args&&... params) {
        memory_storage.check_results(rank, output, mem_idx, funct, std::forward<Args>(params)...);
    }

    std::vector<DType> get_memory(const size_t rank, size_t mem_idx) {
        return memory_storage.get_memory(rank, mem_idx);
    }

private:
    size_t comm_size;
    handles_storage<native_type> memory_storage;
    handles_storage<int> flags_storage;
    std::map<int, std::vector<int>> comm_param_storage;
};

template <class DType>
std::vector<DType> get_initial_send_values(size_t elem_count) {
    std::vector<DType> res(elem_count);
    ccl::datatype dt = ccl::native_type_info<typename std::remove_pointer<DType>::type>::dtype;
    if (dt == ccl::datatype::bfloat16) {
        for (size_t elem_idx = 0; elem_idx < elem_count; elem_idx++) {
            res[elem_idx] = fp32_to_bf16(0.00001 * elem_idx).get_data();
        }
    }
    else if (dt == ccl::datatype::float16) {
        for (size_t elem_idx = 0; elem_idx < elem_count; elem_idx++) {
            res[elem_idx] = fp32_to_fp16(0.0001 * elem_idx).get_data();
        }
    }
    else {
        std::iota(res.begin(), res.end(), 1);
    }
    return res;
}

template <class DType>
bool compare_buffers(std::vector<DType> expected,
                     std::vector<DType> actual,
                     std::stringstream& ss) {
    bool res = true;

    if (expected.size() != actual.size()) {
        ss << "\nunexpected buffer size: expected " << expected.size() << ", got " << actual.size()
           << "\n";
        res = false;
    }

    for (size_t idx = 0; idx < expected.size(); idx++) {
        if (expected[idx] != actual[idx]) {
            ss << "\nunexpected value: idx " << idx << ", expected " << expected[idx] << ", got "
               << actual[idx] << "\n";
            res = false;
            break;
        }
    }

    if (!res) {
        ss << "\nfull content:\n";
        ss << "\nexpected buffer:\n";
        std::copy(expected.begin(), expected.end(), std::ostream_iterator<DType>(ss, " "));
        ss << "\nactual buffer:\n";
        std::copy(actual.begin(), actual.end(), std::ostream_iterator<DType>(ss, " "));
    }

    return res;
}
