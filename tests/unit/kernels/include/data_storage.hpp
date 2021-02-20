#pragma once
#include <map>
#include <vector>
#include "kernel_utils.hpp"

template <class DType>
class data_storage {
public:
    using native_type = DType;
    enum { data_count_max_limit = 100 };

    void initialize_data_storage(size_t threads_num) {
        threads_count = threads_num;
        memory_storage = handles_storage<native_type>(data_count_max_limit * threads_count);
        flags_storage = handles_storage<int>(data_count_max_limit * threads_count);
    }

    // Creators
    template <class... Handles>
    void register_shared_memories_data(size_t thread_idx, Handles&&... h) {
        if (thread_idx >= threads_count) {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                     " - invalid thread requested: " + std::to_string(thread_idx) +
                                     ", total threads count: " + std::to_string(threads_count));
        }
        memory_storage.register_shared_data(thread_idx, threads_count, std::forward<Handles>(h)...);
    }

    template <class... Handles>
    void register_shared_flags_data(size_t thread_idx, Handles&&... h) {
        if (thread_idx >= threads_count) {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                     " - invalid thread requested: " + std::to_string(thread_idx) +
                                     ", total threads count: " + std::to_string(threads_count));
        }
        flags_storage.register_shared_data(thread_idx, threads_count, std::forward<Handles>(h)...);
    }

    template <class... Handles>
    void register_shared_comm_data(size_t thread_idx, Handles&&... h) {
        if (thread_idx >= threads_count) {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                     " - invalid thread requested: " + std::to_string(thread_idx) +
                                     ", total threads count: " + std::to_string(threads_count));
        }

        std::array<int, sizeof...(h)> list{ static_cast<int>(h)... };
        comm_param_storage[thread_idx].insert(
            comm_param_storage[thread_idx].end(), list.begin(), list.end());
    }

    void finalize_data_registration(size_t comm_group_size,
                                    size_t mem_group_size,
                                    size_t flag_group_size) {
        for (size_t rank_idx = 0; rank_idx < threads_count; rank_idx++) {
            (void)comm_group_size;
            memory_storage.rotate_shared_data(rank_idx, threads_count, mem_group_size);
            flags_storage.rotate_shared_data(rank_idx, threads_count, flag_group_size);
        }
    }

    // Getters
    size_t get_threads_count() const {
        return threads_count;
    }

    typename handles_storage<native_type>::mem_handles_container& get_memory_handles(
        size_t thread_idx) {
        if (thread_idx >= threads_count) {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                     " - invalid thread requested: " + std::to_string(thread_idx) +
                                     ", total threads count: " + std::to_string(threads_count));
        }
        return find_storage_val(memory_storage.per_thread_storage, thread_idx);
    }

    handles_storage<native_type>& get_memory_storage() {
        return memory_storage;
    }

    typename handles_storage<int>::mem_handles_container& get_flag_handles(size_t thread_idx) {
        if (thread_idx >= threads_count) {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                     " - invalid thread requested: " + std::to_string(thread_idx) +
                                     ", total threads count: " + std::to_string(threads_count));
        }
        return find_storage_val(flags_storage.per_thread_storage, thread_idx);
    }

    std::vector<int>& get_comm_handles(size_t thread_idx) {
        if (thread_idx >= threads_count) {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                     " - invalid thread requested: " + std::to_string(thread_idx) +
                                     ", total threads count: " + std::to_string(threads_count));
        }
        return find_storage_val(comm_param_storage, thread_idx);
    }

    // Utils
    void dump_memory(std::ostream& out, bool handles_only = false) {
        memory_storage.dump(out, handles_only);
    }

    void dump_memory_by_index(std::ostream& out, size_t mem_handle_index) {
        memory_storage.dump_by_index(out, mem_handle_index);
    }

    // Checking test results
    template <class CheckFunctor, class... Args>
    void check_test_results(
        const size_t& thread_idx,
        std::ostream& output,
        size_t mem_idx, /* offset into mem_handles: 0 - send buffer, 1 - recv buffer*/
        CheckFunctor funct,
        Args&&... params) {
        memory_storage.check_results(
            thread_idx, output, mem_idx, funct, std::forward<Args>(params)...);
    }

private:
    size_t threads_count;
    handles_storage<native_type> memory_storage;
    handles_storage<int> flags_storage;
    std::map<int, std::vector<int>> comm_param_storage;
};
