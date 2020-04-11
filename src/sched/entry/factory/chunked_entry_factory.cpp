#include "sched/entry/factory/chunked_entry_factory.hpp"

namespace entry_factory
{
    void make_chunked_send_entry(ccl_sched* sched,
                                 const ccl_buffer buf,
                                 size_t cnt,
                                 const ccl_datatype& dtype,
                                 size_t dst,
                                 ccl_comm* comm)
    {
        CCL_CHUNKED_ENTRY_FUNCTION("send", dtype, cnt,
            make_entry<send_entry>(sched,
                                   buf + chunk_offset,
                                   chunk_size,
                                   dtype,
                                   dst,
                                   comm));
    }

    void make_chunked_recv_entry(ccl_sched* sched,
                                 const ccl_buffer buf,
                                 size_t cnt,
                                 const ccl_datatype& dtype,
                                 size_t src,
                                 ccl_comm* comm)
    {
        CCL_CHUNKED_ENTRY_FUNCTION("recv", dtype, cnt,
            make_entry<recv_entry>(sched,
                                   buf + chunk_offset,
                                   chunk_size,
                                   dtype,
                                   src,
                                   comm));
    }

    void make_chunked_recv_reduce_entry(ccl_sched* sched,
                                        ccl_buffer inout_buf,
                                        size_t cnt,
                                        size_t* out_cnt,
                                        const ccl_datatype& dtype,
                                        ccl_reduction_t reduction_op,
                                        size_t src,
                                        ccl_buffer comm_buf,
                                        ccl_comm* comm,
                                        ccl_recv_reduce_result_buf_type result_buf_type)
    {
        CCL_CHUNKED_ENTRY_FUNCTION("recv_reduce", dtype, cnt,
            make_entry<recv_reduce_entry>(sched,
                                          inout_buf + chunk_offset,
                                          chunk_size,
                                          out_cnt,
                                          dtype,
                                          reduction_op,
                                          src,
                                          comm_buf + chunk_offset,
                                          comm,
                                          result_buf_type));
    }
}
