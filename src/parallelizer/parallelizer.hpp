#pragma once

#include "sched/sched.hpp"

class mlsl_parallelizer
{
public:
    mlsl_parallelizer(size_t max_data_partition_count)
        : max_data_partition_count(max_data_partition_count)
    {}

    ~mlsl_parallelizer()
    {}

    mlsl_parallelizer(const mlsl_parallelizer& other) = delete;
    mlsl_parallelizer& operator= (const mlsl_parallelizer& other) = delete;

    mlsl_status_t process(mlsl_sched* sched);

private:
	size_t max_data_partition_count;
};
