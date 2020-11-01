#pragma once

struct allgatherv_strategy_impl {

    std::vector<size_t> recv_counts;

    allgatherv_strategy_impl() {
        recv_counts.resize(transport_data::get_comm_size());
    }

    allgatherv_strategy_impl(const allgatherv_strategy_impl&) = delete;
    allgatherv_strategy_impl& operator=(const allgatherv_strategy_impl&) = delete;

    ~allgatherv_strategy_impl() {}

    static constexpr const char* class_name() {
        return "allgatherv";
    }

    size_t get_send_multiplier() {
        return 1;
    }

    size_t get_recv_multiplier() {
        return transport_data::get_comm_size();
    }

    static const ccl::allgatherv_attr& get_op_attr(const bench_exec_attr& bench_attr) {
        return bench_attr.get_attr<ccl::allgatherv_attr>();
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
            recv_counts[idx] = count;
        }
        reqs.push_back(
            ccl::allgatherv(send_buf, count, recv_buf, recv_counts, comm, std::forward<Args>(args)...));
    }
};
