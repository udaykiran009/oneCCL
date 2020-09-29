#pragma once

#include "cpu_coll.hpp"
#include "reduce_scatter_strategy.hpp"

struct reduce_scatter_strategy_impl {
    static constexpr const char* class_name() {
        return "reduce_scatter";
    }

    static const ccl::reduce_scatter_attr& get_op_attr(const bench_exec_attr& bench_attr) {
        return bench_attr.get_attr<ccl::reduce_scatter_attr>();
    }

    template <class Dtype, class comm_t, class... Args>
    void start_internal(comm_t& comm,
                        size_t send_count,
                        const Dtype send_buf,
                        Dtype recv_buf,
                        const bench_exec_attr& bench_attr,
                        req_list_t& reqs,
                        Args&&... args) {

        size_t recv_count = send_count / comm.size();

        reqs.push_back(ccl::reduce_scatter(send_buf,
                                           recv_buf,
                                           recv_count,
                                           bench_attr.reduction,
                                           comm,
                                           std::forward<Args>(args)...));
    }
};
