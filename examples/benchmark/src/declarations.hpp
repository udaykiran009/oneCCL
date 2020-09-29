#pragma once

/* allgatherv implementation */
#include "allgatherv/allgatherv_strategy.hpp"
#include "allgatherv/cpu_allgatherv_coll.hpp"
#include "allgatherv/sycl_allgatherv_coll.hpp"

/* allreduce implementation */
#include "allreduce/allreduce_strategy.hpp"
#include "allreduce/cpu_allreduce_coll.hpp"
#include "allreduce/sycl_allreduce_coll.hpp"

/* alltoall implementation */
#include "alltoall/alltoall_strategy.hpp"
#include "alltoall/cpu_alltoall_coll.hpp"
#include "alltoall/sycl_alltoall_coll.hpp"

/* alltoallv implementation */
#include "alltoallv/alltoallv_strategy.hpp"
#include "alltoallv/cpu_alltoallv_coll.hpp"
#include "alltoallv/sycl_alltoallv_coll.hpp"

/* bcast implementation */
#include "bcast/bcast_strategy.hpp"
#include "bcast/cpu_bcast_coll.hpp"
#include "bcast/sycl_bcast_coll.hpp"

/* reduce implementation */
#include "reduce/reduce_strategy.hpp"
#include "reduce/cpu_reduce_coll.hpp"
#include "reduce/sycl_reduce_coll.hpp"

/* reduce_scatter implementation */
#include "reduce_scatter/reduce_scatter_strategy.hpp"
#include "reduce_scatter/cpu_reduce_scatter_coll.hpp"
#include "reduce_scatter/sycl_reduce_scatter_coll.hpp"

/* sparse_allreduce implementation */
#include "sparse_allreduce/sparse_allreduce_base.hpp"
#include "sparse_allreduce/sparse_allreduce_strategy.hpp"
#include "sparse_allreduce/cpu_sparse_allreduce_coll.hpp"
#include "sparse_allreduce/sycl_sparse_allreduce_coll.hpp"
