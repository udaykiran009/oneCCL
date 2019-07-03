#pragma once

#include "sched/sched.hpp"

class iccl_parallelizer
{
public:
    iccl_parallelizer(size_t max_data_partition_count)
        : max_data_partition_count(max_data_partition_count)
    {}

    ~iccl_parallelizer() = default;

    iccl_parallelizer(const iccl_parallelizer& other) = delete;
    iccl_parallelizer& operator= (const iccl_parallelizer& other) = delete;

    iccl_parallelizer(iccl_parallelizer&& other) = delete;
    iccl_parallelizer& operator= (iccl_parallelizer&& other) = delete;

    iccl_status_t process(iccl_sched* sched);

private:
    size_t max_data_partition_count;
};
