#pragma once

#include "coll/coll.hpp"

union ccl_coll_algorithm
{
   ccl_coll_allgatherv_algo allgatherv;
   ccl_coll_allreduce_algo allreduce;
   ccl_coll_barrier_algo barrier;
   ccl_coll_bcast_algo bcast;
   ccl_coll_reduce_algo reduce;
   ccl_coll_sparse_allreduce_algo sparse_allreduce;
};

struct ccl_coll_algorithm_selector_result
{
    ccl_coll_type ctype;
    ccl_coll_algorithm algo;
};

class ccl_coll_algorithm_selector
{

public:
    ccl_coll_algorithm_selector(const ccl_coll_algorithm_selector& other) = delete;
    const ccl_coll_algorithm_selector& operator=(const ccl_coll_algorithm_selector& other) = delete;
    ccl_coll_algorithm_selector() {}
    ~ccl_coll_algorithm_selector() {}

    void /*ccl_coll_algorithm_selector_result*/ get(const ccl_coll_param& param) { return; }

private:

};
