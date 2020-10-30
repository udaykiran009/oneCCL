#pragma once

struct alltoallv_strategy_impl {
    int comm_size = 0;
    std::vector<size_t> send_counts;
    std::vector<size_t> recv_counts;

    alltoallv_strategy_impl(size_t size) : comm_size(size) {
        send_counts.resize(comm_size);
        recv_counts.resize(comm_size);
    }

    alltoallv_strategy_impl(const alltoallv_strategy_impl&) = delete;
    alltoallv_strategy_impl& operator=(const alltoallv_strategy_impl&) = delete;

    ~alltoallv_strategy_impl() {}

    static constexpr const char* class_name() {
        return "alltoallv";
    }

    static const ccl::alltoallv_attr& get_op_attr(const bench_exec_attr& bench_attr) {
        return bench_attr.get_attr<ccl::alltoallv_attr>();
    }

    template <class Dtype, class... Args>
    void start_internal(ccl::communicator& comm,
                        size_t count,
                        const Dtype send_buf,
                        Dtype recv_buf,
                        const bench_exec_attr& bench_attr,
                        req_list_t& reqs,
                        Args&&... args) {
        for (int idx = 0; idx < comm_size; idx++) {
            send_counts[idx] = count;
            recv_counts[idx] = count;
        }

        reqs.push_back(ccl::alltoallv(
            send_buf, send_counts, recv_buf, recv_counts, comm, std::forward<Args>(args)...));
    }
};
