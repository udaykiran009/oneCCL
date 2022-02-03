#pragma once

#include "sched/sched.hpp"
#include "internal_types.hpp"

class ccl_parallelizer {
public:
    ccl_parallelizer(size_t max_data_partition_count)
            : max_data_partition_count(max_data_partition_count) {}

    ~ccl_parallelizer() = default;

    ccl_parallelizer(const ccl_parallelizer& other) = delete;
    ccl_parallelizer& operator=(const ccl_parallelizer& other) = delete;

    ccl_parallelizer(ccl_parallelizer&& other) = delete;
    ccl_parallelizer& operator=(ccl_parallelizer&& other) = delete;

    ccl::status process(ccl_sched* sched);

private:
    ccl::status process_deps(ccl_sched* sched);

#ifdef CCL_ENABLE_SYCL
    ccl::status process_pre_post_copies(ccl_sched* sched);
    ccl::status process_output_event(ccl_sched* sched);
#endif // CCL_ENABLE_SYCL

    ccl::status process_base(ccl_sched* sched);

    size_t max_data_partition_count;
};
