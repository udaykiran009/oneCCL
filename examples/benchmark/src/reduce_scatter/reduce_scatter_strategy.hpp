#pragma once

#include "cpu_coll.hpp"
#include "reduce_scatter_strategy.hpp"

struct reduce_scatter_strategy_impl {
    static constexpr const char* class_name() {
        return "reduce_scatter";
    }

    size_t get_send_multiplier() {
        return 1;
    }

    size_t get_recv_multiplier() {
        return 1;
    }

    static const ccl::reduce_scatter_attr& get_op_attr(const bench_exec_attr& bench_attr) {
        return bench_attr.get_attr<ccl::reduce_scatter_attr>();
    }

    template <class Dtype, class... Args>
    void start_internal(ccl::communicator& comm,
                        size_t send_count,
                        const Dtype send_buf,
                        Dtype recv_buf,
                        const bench_exec_attr& bench_attr,
                        req_list_t& reqs,
                        Args&&... args) {
        size_t recv_count = send_count / comm.size();

        if (recv_count == 0) {
            reqs.push_back(ccl::event());
            return;
        }

        reqs.push_back(ccl::reduce_scatter(send_buf,
                                           recv_buf,
                                           recv_count,
                                           get_ccl_dtype<Dtype>(),
                                           bench_attr.reduction,
                                           comm,
                                           std::forward<Args>(args)...));
    }
};
