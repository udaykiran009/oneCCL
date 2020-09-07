#pragma once

#include "common/comm/comm.hpp"
#include "common/utils/buffer.hpp"
#include "sched/sched.hpp"

class ccl_allreduce_2d_builder {
public:
    ccl_allreduce_2d_builder(size_t base_size, bool switch_dims);
    ~ccl_allreduce_2d_builder();

    ccl_allreduce_2d_builder(const ccl_allreduce_2d_builder&) = delete;
    ccl_allreduce_2d_builder(ccl_allreduce_2d_builder&&) = delete;

    ccl_allreduce_2d_builder& operator=(const ccl_allreduce_2d_builder&) = delete;
    ccl_allreduce_2d_builder& operator=(ccl_allreduce_2d_builder&&) = delete;

    ccl_status_t build(ccl_sched* sched,
                       ccl_buffer send_buf,
                       ccl_buffer recv_buf,
                       size_t count,
                       const ccl_datatype& dtype,
                       ccl::reduction op,
                       ccl_comm* comm);

    ccl_comm* get_first_dim_comm() const {
        return first_dim_comm.get();
    }
    ccl_comm* get_second_dim_comm() const {
        return second_dim_comm.get();
    }

private:
    std::shared_ptr<ccl_comm> first_dim_comm;
    std::shared_ptr<ccl_comm> second_dim_comm;
};
