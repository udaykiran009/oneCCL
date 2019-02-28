#pragma once

#include <unordered_map>
#include <list>

typedef std::unordered_map<size_t, size_t> idx_offset_map;

struct mlsl_sparse_allreduce_handler
{
   size_t val_dim_cnt;
   size_t recv_buff_size;
   size_t send_buff_size;
   size_t itype_size;
   size_t vtype_size;

   size_t send_count[2];
   size_t dst_count[2];

   void* dst_buf;
   void* send_tmp_buf;
   void** recv_buf;
   void** recv_vbuf;
   size_t* recv_icount;
   size_t* recv_vcount;

   idx_offset_map *iv_map;
   std::list<mlsl_sched_buffer_handler>* buf_list;
};