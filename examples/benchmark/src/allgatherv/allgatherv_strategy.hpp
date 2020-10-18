#pragma once

struct allgatherv_strategy_impl {

    size_t comm_size = 0;
    std::vector<size_t> recv_counts;

    allgatherv_strategy_impl(size_t size) : comm_size(size) {
        recv_counts.resize(size);
    }

    allgatherv_strategy_impl(const allgatherv_strategy_impl&) = delete;
    allgatherv_strategy_impl& operator=(const allgatherv_strategy_impl&) = delete;

    ~allgatherv_strategy_impl() {}

    static constexpr const char* class_name() {
        return "allgatherv";
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
        for (size_t idx = 0; idx < comm_size; idx++) {
            recv_counts[idx] = count;
        }
        reqs.push_back(
            ccl::allgatherv(send_buf, count, recv_buf, recv_counts, comm, std::forward<Args>(args)...));
    }
};
