#pragma once

#include <unordered_map>

typedef std::map<size_t, std::vector<size_t> > idx_offset_map;

struct ccl_sparse_allreduce_handler
{
   size_t val_dim_cnt;
   size_t recv_buf_count;
   size_t itype_size;
   size_t vtype_size;
   size_t comm_size;
   size_t buf_size;
   size_t recv_from;
   size_t iter; /*iteration within ring algorithm*/

   size_t send_count[2];
   size_t dst_count[2];

   /* explicit index/value buffer for allgatherv algo */
   void* dst_ibuf;
   void* dst_vbuf;

   /* TODO: refactor ring and mask to enable alloc_fn */
   void* dst_buf;

   void* send_tmp_buf;
   void* recv_buf;
   void** recv_ibuf;
   void** recv_vbuf;
   size_t* recv_icount;
   size_t* recv_vcount;
   size_t* size_per_rank;
   size_t* recv_counts;

   void* all_idx_buf;
   void* all_val_buf;

   void* send_ibuf;
   void* send_vbuf;

   float* tmp;
   float* acc;

   ccl_datatype value_dtype;
   ccl_datatype index_dtype;
   ccl::reduction op;

   std::unique_ptr<idx_offset_map> iv_map;
   ccl_sched* sched;
   ccl_comm* comm;
};

