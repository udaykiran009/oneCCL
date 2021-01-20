#pragma once

struct allreduce_strategy_impl {
    static constexpr const char* class_name() {
        return "allreduce";
    }

    size_t get_send_multiplier() {
        return 1;
    }

    size_t get_recv_multiplier() {
        return 1;
    }

    static const ccl::allreduce_attr& get_op_attr(const bench_exec_attr& bench_attr) {
        return bench_attr.get_attr<ccl::allreduce_attr>();
    }

    template <class Dtype, class... Args>
    void start_internal(ccl::communicator& comm,
                        size_t count,
                        const Dtype send_buf,
                        Dtype recv_buf,
                        const bench_exec_attr& bench_attr,
                        req_list_t& reqs,
                        Args&&... args) {
        reqs.push_back(ccl::allreduce(send_buf,
                                      recv_buf,
                                      count,
                                      get_ccl_dtype<Dtype>(),
                                      bench_attr.reduction,
                                      comm,
                                      std::forward<Args>(args)...));
    }
};
