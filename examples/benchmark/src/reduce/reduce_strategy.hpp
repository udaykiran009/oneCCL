#ifndef REDUCE_STRATEGY_HPP
#define REDUCE_STRATEGY_HPP

#include "cpu_coll.hpp"
#include "reduce_strategy.hpp"

struct reduce_strategy_impl {
    static constexpr const char* class_name() {
        return "reduce";
    }

    template <class Dtype>
    void start_internal(ccl::communicator& comm,
                        size_t count,
                        const Dtype send_buf,
                        Dtype recv_buf,
                        const bench_coll_exec_attr& bench_attr,
                        ccl::stream_t& stream,
                        req_list_t& reqs) {
        reqs.push_back(comm.reduce(send_buf,
                                   recv_buf,
                                   count,
                                   bench_attr.reduction,
                                   COLL_ROOT,
                                   &bench_attr.coll_attr,
                                   stream));
    }
};

#endif /* REDUCE_STRATEGY_HPP */
