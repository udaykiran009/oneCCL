#pragma once

struct bcast_strategy_impl {
    static constexpr const char* class_name() {
        return "bcast";
    }

    size_t get_send_multiplier() {
        return 1;
    }

    size_t get_recv_multiplier() {
        return 1;
    }

    static const ccl::broadcast_attr& get_op_attr(const bench_exec_attr& bench_attr) {
        return bench_attr.get_attr<ccl::broadcast_attr>();
    }

    template <class Dtype, class... Args>
    void start_internal(ccl::communicator& comm,
                        size_t count,
                        Dtype send_buf,
                        Dtype recv_buf,
                        const bench_exec_attr& bench_attr,
                        req_list_t& reqs,
                        Args&&... args) {
        (void)send_buf;
        reqs.push_back(ccl::broadcast(
            recv_buf, count, get_ccl_dtype<Dtype>(), COLL_ROOT, comm, std::forward<Args>(args)...));
    }
};
