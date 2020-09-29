#pragma once

#define ALIGNMENT      (4096)
#define DTYPE          float

#define ALL_DTYPES_LIST     "char,int,float,double,int64_t,uint64_t"
#define ALL_REDUCTIONS_LIST "sum,prod,min,max"

#define DEFAULT_BACKEND         ccl::stream_type::host
#define DEFAULT_LOOP            LOOP_REGULAR
#define DEFAULT_COLL_LIST \
    "allgatherv,allreduce,alltoall,alltoallv,bcast,reduce," \
    "reduce_scatter,sparse_allreduce,sparse_allreduce_bf16," \
    "allgatherv,allreduce,alltoall,alltoallv,bcast,reduce," \
    "reduce_scatter,sparse_allreduce,sparse_allreduce_bf16"
#define DEFAULT_ITERS           (16)
#define DEFAULT_WARMUP_ITERS    (16)
#define DEFAULT_BUF_COUNT       (16)
#define DEFAULT_MIN_ELEM_COUNT  (1)
#define DEFAULT_MAX_ELEM_COUNT  (128)
#define DEFAULT_CHECK_VALUES    (1)
#define DEFAULT_BUF_TYPE        BUF_MULTI
#define DEFAULT_V2I_RATIO       (128)
#define DEFAULT_DTYPES_LIST     "float"
#define DEFAULT_REDUCTIONS_LIST "sum"
#define DEFAULT_CSV_FILEPATH    ""
