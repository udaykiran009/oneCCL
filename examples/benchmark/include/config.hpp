#pragma once

#define ALIGNMENT (4096)
#define DTYPE     float

#define ALL_COLLS_LIST      "allgatherv,allreduce,alltoall,alltoallv,bcast,reduce,reduce_scatter"
#define ALL_DTYPES_LIST     "int8,int32,int64,uint64,float32,float64"
#define ALL_REDUCTIONS_LIST "sum,prod,min,max"

#define DEFAULT_BACKEND        BACKEND_HOST
#define DEFAULT_LOOP           LOOP_REGULAR
#define DEFAULT_ITERS          (16)
#define DEFAULT_WARMUP_ITERS   (16)
#define DEFAULT_BUF_COUNT      (16)
#define DEFAULT_MIN_ELEM_COUNT (1)
#define DEFAULT_MAX_ELEM_COUNT (128)
#define DEFAULT_CHECK_VALUES   (1)
#define DEFAULT_V2I_RATIO      (128)
#define DEFAULT_SYCL_DEV_TYPE  SYCL_DEV_GPU
#define DEFAULT_SYCL_MEM_TYPE  SYCL_MEM_USM
#define DEFAULT_SYCL_USM_TYPE  SYCL_USM_DEVICE
#define DEFAULT_RANKS_PER_PROC (1)

#define DEFAULT_COLL_LIST       "allreduce"
#define DEFAULT_DTYPES_LIST     "float32"
#define DEFAULT_REDUCTIONS_LIST "sum"
#define DEFAULT_CSV_FILEPATH    ""
