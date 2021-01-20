#pragma once

struct alltoallv_strategy_impl {
    std::vector<size_t> send_counts;
    std::vector<size_t> recv_counts;

    alltoallv_strategy_impl() {
        send_counts.resize(transport_data::get_comm_size());
        recv_counts.resize(transport_data::get_comm_size());
    }

    alltoallv_strategy_impl(const alltoallv_strategy_impl&) = delete;
    alltoallv_strategy_impl& operator=(const alltoallv_strategy_impl&) = delete;

    ~alltoallv_strategy_impl() {}

    static constexpr const char* class_name() {
        return "alltoallv";
    }

    size_t get_send_multiplier() {
        return transport_data::get_comm_size();
    }

    size_t get_recv_multiplier() {
        return transport_data::get_comm_size();
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
        for (int idx = 0; idx < comm.size(); idx++) {
            send_counts[idx] = count;
            recv_counts[idx] = count;
        }

        reqs.push_back(ccl::alltoallv(send_buf,
                                      send_counts,
                                      recv_buf,
                                      recv_counts,
                                      get_ccl_dtype<Dtype>(),
                                      comm,
                                      std::forward<Args>(args)...));
    }
};
