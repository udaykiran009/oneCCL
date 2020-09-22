#ifndef REDUCE_STRATEGY_HPP
#define REDUCE_STRATEGY_HPP

#include "cpu_coll.hpp"
#include "reduce_strategy.hpp"

struct reduce_strategy_impl {
    static constexpr const char* class_name() {
        return "reduce";
    }

    static const ccl::reduce_attr& get_op_attr(const bench_coll_exec_attr& bench_attr) {
        return bench_attr.get_attr<ccl::reduce_attr>();
    }

    template <class Dtype, class comm_t, class... Args>
    void start_internal(comm_t& comm,
                        size_t count,
                        const Dtype send_buf,
                        Dtype recv_buf,
                        const bench_coll_exec_attr& bench_attr,
                        req_list_t& reqs,
                        Args&&... args) {
        reqs.push_back(comm.reduce(send_buf,
                                   recv_buf,
                                   count,
                                   bench_attr.reduction,
                                   COLL_ROOT,
                                   std::forward<Args>(args)...));
    }
};

#endif /* REDUCE_STRATEGY_HPP */
