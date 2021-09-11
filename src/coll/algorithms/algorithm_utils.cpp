#include "coll/algorithms/algorithms_enum.hpp"

bool ccl_coll_type_is_reduction(ccl_coll_type ctype) {
    switch (ctype) {
        case ccl_coll_allreduce:
        case ccl_coll_reduce:
        case ccl_coll_reduce_scatter: return true;
        default: return false;
    }
}

const char* ccl_coll_type_to_str(ccl_coll_type type) {
    switch (type) {
        case ccl_coll_allgatherv: return "allgatherv";
        case ccl_coll_allreduce: return "allreduce";
        case ccl_coll_alltoall: return "alltoall";
        case ccl_coll_alltoallv: return "alltoallv";
        case ccl_coll_barrier: return "barrier";
        case ccl_coll_bcast: return "bcast";
        case ccl_coll_reduce: return "reduce";
        case ccl_coll_reduce_scatter: return "reduce_scatter";
        case ccl_coll_sparse_allreduce: return "sparse_allreduce";
        case ccl_coll_partial: return "partial";
        case ccl_coll_undefined: return "undefined";
        default: return "unknown";
    }
    return "unknown";
}