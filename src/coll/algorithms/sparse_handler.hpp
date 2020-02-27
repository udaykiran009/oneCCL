#pragma once

#include <unordered_map>

typedef std::unordered_map<size_t, size_t> idx_offset_map;

struct ccl_sparse_allreduce_handler
{
   size_t val_dim_cnt;
   size_t recv_buf_size;
   size_t send_buf_size;
   size_t itype_size;
   size_t vtype_size;
   size_t comm_size;
   size_t recv_count;
   size_t buf_size;

   size_t send_count[2];
   size_t dst_count[2];

   void* dst_buf;
   void* send_tmp_buf;
   void** recv_buf;
   void** recv_ibuf;
   void** recv_vbuf;
   size_t* recv_sizes;
   size_t* recv_icount;
   size_t* recv_vcount;
   size_t* size_per_rank;
   size_t* recv_counts;

   void* all_idx_buf;
   void* all_val_buf;

   ccl_datatype_internal_t value_dtype;
   ccl_reduction_t op;

   std::unique_ptr<idx_offset_map> iv_map;
   ccl_sched* sched;
   ccl_comm* comm;
};

