#pragma once
#include "ccl_types.h"

class ccl_common_op_attr_impl_t
{
    ccl_prologue_fn_t prologue_fn;
    ccl_epilogue_fn_t epilogue_fn;

    /* Priority for collective operation */
    size_t priority;

    /* Blocking/non-blocking */
    int synchronous;

    /* Persistent/non-persistent */
    int to_cache;


    /**
     * Id of the operation. If specified, new communicator will be created and collective
     * operations with the same @b match_id will be executed in the same order.
     */
    const char* match_id;
};
