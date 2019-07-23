#include "base.h"

#include <string>

#define RUN_COLLECTIVE(start_cmd, name, expected)                          \
  do {                                                                     \
      t = 0;                                                               \
      for (iter_idx = 0; iter_idx < ITERS; iter_idx++)                     \
      {                                                                    \
          for (idx = 0; idx < COUNT; idx++)                                \
          {                                                                \
              send_buf[idx] = (float)rank;                                 \
              recv_buf[idx] = 0.0;                                         \
          }                                                                \
          t1 = when();                                                     \
          CCL_CALL(start_cmd);                                             \
          CCL_CALL(ccl_wait(request));                                     \
          t2 = when();                                                     \
          t += (t2 - t1);                                                  \
      }                                                                    \
      ccl_barrier(NULL);                                                   \
      for (idx = 0; idx < COUNT; idx++)                                    \
      {                                                                    \
          if (recv_buf[idx] != expected)                                   \
          {                                                                \
              printf("iter %zu, idx %zu, expected %f, got %f\n",           \
                      iter_idx, idx, expected, recv_buf[idx]);             \
              ASSERT(0, "unexpected value");                               \
          }                                                                \
      }                                                                    \
      printf("[%zu] avg %s time: %8.2lf us\n", rank, name, t / ITERS);     \
      fflush(stdout);                                                      \
  } while (0)

ccl_status_t do_prologue_float_2x(const void* in_buf, size_t in_count, ccl_datatype_t in_dtype,
                                  void** out_buf, size_t* out_count, ccl_datatype_t* out_dtype,
                                  size_t* out_dtype_size)
{
    size_t idx;
    ASSERT(in_dtype == ccl_dtype_float, "unexpected dtype %d", in_dtype);
    ASSERT(out_buf, "null ptr");

    if (out_buf) *out_buf = (void*)in_buf;
    if (out_count) *out_count = in_count;
    if (out_dtype) *out_dtype = ccl_dtype_float;
    if (out_dtype_size) *out_dtype_size = 4;

    for (idx = 0; idx < in_count; idx++)
    {
        ((float*)(*out_buf))[idx] = ((float*)in_buf)[idx] * 2;
    }
    return ccl_status_success;
}

ccl_status_t do_epilogue_float_2x(const void* in_buf, size_t in_count, ccl_datatype_t in_dtype,
                                  void* out_buf, size_t* out_count, ccl_datatype_t out_dtype)
{
    size_t idx;
    ASSERT(in_dtype == ccl_dtype_float, "unexpected dtype %d", in_dtype);
    ASSERT(out_dtype == ccl_dtype_float, "unexpected dtype %d", out_dtype);
    if (out_count) *out_count = in_count;
    for (idx = 0; idx < in_count; idx++)
    {
        ((float*)out_buf)[idx] = ((float*)in_buf)[idx] * 2;
    }
    return ccl_status_success;
}

ccl_status_t do_prologue_float_to_char(const void* in_buf, size_t in_count, ccl_datatype_t in_dtype,
                                       void** out_buf, size_t* out_count, ccl_datatype_t* out_dtype,
                                       size_t* out_dtype_size)
{
    size_t idx;
    ASSERT(in_dtype == ccl_dtype_float, "unexpected dtype %d", in_dtype);
    ASSERT(out_buf, "null ptr");

    if (out_buf) *out_buf = malloc(in_count);
    if (out_count) *out_count = in_count;
    if (out_dtype) *out_dtype = ccl_dtype_char;
    if (out_dtype_size) *out_dtype_size = 1;

    for (idx = 0; idx < in_count; idx++)
    {
        float fval = ((float*)in_buf)[idx];
        int ival = (int)fval;
        ((char*)(*out_buf))[idx] = (char)(ival % 256);
    }

    return ccl_status_success;
}

ccl_status_t do_epilogue_char_to_float(const void* in_buf, size_t in_count, ccl_datatype_t in_dtype,
                                       void* out_buf, size_t* out_count, ccl_datatype_t out_dtype)
{
    size_t idx;
    ASSERT(in_dtype == ccl_dtype_char, "unexpected dtype %d", in_dtype);
    ASSERT(out_dtype == ccl_dtype_float, "unexpected dtype %d", out_dtype);
    if (out_count) *out_count = in_count;
    for (idx = 0; idx < in_count; idx++)
    {
        ((float*)out_buf)[idx] = (float)(((char*)in_buf)[idx]);
    }
    if (in_buf != out_buf) free((void*)in_buf);
    return ccl_status_success;
}

ccl_status_t do_reduction_sum(const void* in_buf, size_t in_count, void* inout_buf,
                              size_t* out_count, const void** ctx, ccl_datatype_t dtype)
{
    size_t idx;
    if (out_count) *out_count = in_count;
    switch (dtype)
    {
        case ccl_dtype_char:
            for (idx = 0; idx < in_count; idx++)
            {
                ((char*)inout_buf)[idx] += ((char*)in_buf)[idx];
            }
            break;
        case ccl_dtype_float:
            for (idx = 0; idx < in_count; idx++)
            {
                ((float*)inout_buf)[idx] += ((float*)in_buf)[idx];
            }
            break;
        default:
            ASSERT(0, "unexpected dtype %d", dtype);
            break;
    }
    return ccl_status_success;
}

ccl_status_t do_reduction_null(const void* in_buf, size_t in_count, void* inout_buf,
                               size_t* out_count, const void** ctx, ccl_datatype_t dtype)
{
    size_t idx;
    if (out_count) *out_count = in_count;
    switch (dtype)
    {
        case ccl_dtype_char:
            for (idx = 0; idx < in_count; idx++)
            {
                ((char*)inout_buf)[idx] = (char)0;
            }
            break;
        case ccl_dtype_float:
            for (idx = 0; idx < in_count; idx++)
            {
                ((float*)inout_buf)[idx] = (float)0;
            }
            break;
        default:
            ASSERT(0, "unexpected dtype %d", dtype);
            break;
    }
    return ccl_status_success;
}

int main()
{
    float send_buf[COUNT];
    float recv_buf[COUNT];

    test_init();

    std::string base_match_id(coll_attr.match_id), match_id;

    coll_attr.to_cache = 1;

    /* regular sum allreduce */
    match_id = base_match_id + "_regular";
    coll_attr.match_id = match_id.c_str();
    RUN_COLLECTIVE(ccl_allreduce(send_buf, recv_buf, COUNT, ccl_dtype_float, ccl_reduction_sum, &coll_attr, NULL, &request),
                   "regular_allreduce",
                   ((size - 1) * ((float)size / 2)));

    /* prologue */
    match_id = base_match_id + "_prologue";
    coll_attr.match_id = match_id.c_str();
    coll_attr.prologue_fn = do_prologue_float_2x;
    coll_attr.epilogue_fn = NULL;
    coll_attr.reduction_fn = NULL;
    RUN_COLLECTIVE(ccl_allreduce(send_buf, recv_buf, COUNT, ccl_dtype_float, ccl_reduction_sum, &coll_attr, NULL, &request),
                   "allreduce_with_prologue",
                   (2 * (size - 1) * ((float)size / 2)));

    /* epilogue */
    match_id = base_match_id + "_epilogue";
    coll_attr.match_id = match_id.c_str();
    coll_attr.prologue_fn = NULL;
    coll_attr.epilogue_fn = do_epilogue_float_2x;
    coll_attr.reduction_fn = NULL;
    RUN_COLLECTIVE(ccl_allreduce(send_buf, recv_buf, COUNT, ccl_dtype_float, ccl_reduction_sum, &coll_attr, NULL, &request),
                   "allreduce_with_epilogue",
                   (2 * (size - 1) * ((float)size / 2)));

    /* reduction_sum */
    match_id = base_match_id + "_reduction_sum";
    coll_attr.match_id = match_id.c_str();
    coll_attr.prologue_fn = NULL;
    coll_attr.epilogue_fn = NULL;
    coll_attr.reduction_fn = do_reduction_sum;
    RUN_COLLECTIVE(ccl_allreduce(send_buf, recv_buf, COUNT, ccl_dtype_float, ccl_reduction_custom, &coll_attr, NULL, &request),
                   "allreduce_with_reduction_sum",
                   ((size - 1) * ((float)size / 2)));

    /* reduction_null */
    match_id = base_match_id + "_reduction_null";
    coll_attr.match_id = match_id.c_str();
    coll_attr.prologue_fn = NULL;
    coll_attr.epilogue_fn = NULL;
    coll_attr.reduction_fn = do_reduction_null;
    RUN_COLLECTIVE(ccl_allreduce(send_buf, recv_buf, COUNT, ccl_dtype_float, ccl_reduction_custom, &coll_attr, NULL, &request),
                   "allreduce_with_reduction_null",
                   (float)0);

    /* prologue and epilogue */
    match_id = base_match_id + "_prologue_and_epilogue";
    coll_attr.match_id = match_id.c_str();
    coll_attr.prologue_fn = do_prologue_float_2x;
    coll_attr.epilogue_fn = do_epilogue_float_2x;
    coll_attr.reduction_fn = NULL;
    RUN_COLLECTIVE(ccl_allreduce(send_buf, recv_buf, COUNT, ccl_dtype_float, ccl_reduction_sum, &coll_attr, NULL, &request),
                   "allreduce_with_prologue_and_epilogue",
                   (2 * 2 * (size - 1) * ((float)size / 2)));

    /* prologue and reduction_sum */
    match_id = base_match_id + "_prologue_and_reduction_sum";
    coll_attr.match_id = match_id.c_str();
    coll_attr.prologue_fn = do_prologue_float_2x;
    coll_attr.epilogue_fn = NULL;
    coll_attr.reduction_fn = do_reduction_sum;
    RUN_COLLECTIVE(ccl_allreduce(send_buf, recv_buf, COUNT, ccl_dtype_float, ccl_reduction_custom, &coll_attr, NULL, &request),
                   "allreduce_with_prologue_and_reduction_sum",
                   (2 * (size - 1) * ((float)size / 2)));

    /* epilogue and reduction_sum */
    match_id = base_match_id + "_epilogue_and_reduction_sum";
    coll_attr.match_id = match_id.c_str();
    coll_attr.prologue_fn = NULL;
    coll_attr.epilogue_fn = do_epilogue_float_2x;
    coll_attr.reduction_fn = do_reduction_sum;
    RUN_COLLECTIVE(ccl_allreduce(send_buf, recv_buf, COUNT, ccl_dtype_float, ccl_reduction_custom, &coll_attr, NULL, &request),
                   "allreduce_with_epilogue_and_reduction_sum",
                   (2 * (size - 1) * ((float)size / 2)));

    /* prologue and epilogue and reduction_sum */
    match_id = base_match_id + "_prologue_and_epilogue_reduction_sum";
    coll_attr.match_id = match_id.c_str();
    coll_attr.prologue_fn = do_prologue_float_2x;
    coll_attr.epilogue_fn = do_epilogue_float_2x;
    coll_attr.reduction_fn = do_reduction_sum;
    RUN_COLLECTIVE(ccl_allreduce(send_buf, recv_buf, COUNT, ccl_dtype_float, ccl_reduction_custom, &coll_attr, NULL, &request),
                   "allreduce_with_prologue_and_epilogue_and_reduction_sum",
                   (2 * 2 * (size - 1) * ((float)size / 2)));

    /* prologue and epilogue and reduction_null */
    match_id = base_match_id + "_prologue_and_epilogue_reduction_null";
    coll_attr.match_id = match_id.c_str();
    coll_attr.prologue_fn = do_prologue_float_2x;
    coll_attr.epilogue_fn = do_epilogue_float_2x;
    coll_attr.reduction_fn = do_reduction_null;
    RUN_COLLECTIVE(ccl_allreduce(send_buf, recv_buf, COUNT, ccl_dtype_float, ccl_reduction_custom, &coll_attr, NULL, &request),
                   "allreduce_with_prologue_and_epilogue_and_reduction_null",
                   (float)0);

    /* prologue and epilogue and reduction_sum */
    match_id = base_match_id + "_prologue_and_epilogue_reduction_sum2";
    coll_attr.match_id = match_id.c_str();
    coll_attr.prologue_fn = do_prologue_float_to_char;
    coll_attr.epilogue_fn = do_epilogue_char_to_float;
    coll_attr.reduction_fn = do_reduction_sum;
    RUN_COLLECTIVE(ccl_allreduce(send_buf, recv_buf, COUNT, ccl_dtype_float, ccl_reduction_custom, &coll_attr, NULL, &request),
                   "allreduce_with_prologue_and_epilogue_and_reduction_sum",
                   ((size - 1) * ((float)size / 2)));

    test_finalize();

    return 0;
}
