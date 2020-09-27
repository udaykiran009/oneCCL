#pragma once
#include <stddef.h>
#include <stdint.h>

#ifndef container_of
#define container_of(ptr, type, field) ((type*)((char*)ptr - offsetof(type, field)))
#endif

#ifndef gettid
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#define gettid() syscall(SYS_gettid)
#endif

#define SIZEOFARR(arr) (sizeof(arr) / sizeof(arr[0]))

#define ATL_CACHELINE_LEN     64
#define ATL_REQ_SIZE          8
#define ATL_PROGRESS_MODE_ENV "ATL_PROGRESS_MODE"

#define DIR_SEP  '/'
#define FILENAME (strrchr(__FILE__, DIR_SEP) ? strrchr(__FILE__, DIR_SEP) + 1 : __FILE__)

/*
 * Dynamically loaded transports must export the following entry point.
 * This is invoked by the ATL framework when the transport library is loaded.
 */
#define ATL_EXT_INI \
    __attribute__((visibility("default"))) atl_status_t atl_ini(atl_transport_t* atl_transport)

#define ATL_OFI_INI ATL_EXT_INI
#define ATL_MPI_INI ATL_EXT_INI

typedef struct atl_ctx atl_ctx_t;
typedef struct atl_ep atl_ep_t;

typedef enum { ATL_PROGRESS_POLL, ATL_PROGRESS_CHECK } atl_progress_mode_t;

typedef enum { ATL_RA_WAIT, ATL_RA_RUN, ATL_RA_FINALIZE } atl_resize_action_t;

typedef atl_resize_action_t (*atl_resize_fn_t)(size_t size);

typedef enum {
    ATL_STATUS_SUCCESS,
    ATL_STATUS_FAILURE,
    ATL_STATUS_AGAIN,
    ATL_STATUS_UNSUPPORTED
} atl_status_t;

inline const char* atl_status_to_str(atl_status_t status) {
    switch (status) {
        case ATL_STATUS_SUCCESS: return "SUCCESS";
        case ATL_STATUS_FAILURE: return "FAILURE";
        case ATL_STATUS_UNSUPPORTED: return "UNSUPPORTED";
        default: return "UNKNOWN";
    }
}

typedef enum {
    ATL_DTYPE_CHAR,

    ATL_DTYPE_UINT8, //new
    ATL_DTYPE_INT16, //new
    ATL_DTYPE_UINT16, //new
    ATL_DTYPE_INT, //replace
    ATL_DTYPE_UINT32, //new
    ATL_DTYPE_INT64, //replace
    ATL_DTYPE_UINT64, //replace

    ATL_DTYPE_FLOAT16,
    ATL_DTYPE_FLOAT,
    ATL_DTYPE_DOUBLE,

    ATL_DTYPE_BF16,
} atl_datatype_t;
//#define ccl_dtype_uint8         ((ccl_datatype_t)(1))
//#define ccl_dtype_int16         ((ccl_datatype_t)(2))
//#define ccl_dtype_uint16        ((ccl_datatype_t)(3))
//#define ccl_dtype_int           ((ccl_datatype_t)(4))
//#define ccl_dtype_uint32        ((ccl_datatype_t)(5))
//#define ccl_dtype_int64         ((ccl_datatype_t)(6))
//#define ccl_dtype_uint64        ((ccl_datatype_t)(7))
//
//#define ccl_dtype_bfloat8       ((ccl_datatype_t)(8))
//#define ccl_dtype_bf16         ((ccl_datatype_t)(9))
//#define ccl_dtype_float16       ((ccl_datatype_t)(10))
//#define ccl_dtype_float         ((ccl_datatype_t)(11))
//
//#define ccl_dtype_double        ((ccl_datatype_t)(12))
typedef enum {
    ATL_REDUCTION_SUM,
    ATL_REDUCTION_PROD,
    ATL_REDUCTION_MIN,
    ATL_REDUCTION_MAX,
    ATL_REDUCTION_CUSTOM
} atl_reduction_t;

typedef struct {
    size_t ep_count;
    int enable_shm;
    size_t tag_bits;
    uint64_t max_tag;
    int enable_rma;
    size_t max_order_waw_size;
} atl_attr_t;

typedef struct {
    void* buf;
    size_t len;
    uintptr_t local_key;
    uintptr_t remote_key;
} atl_mr_t;

typedef struct {
    size_t global_idx;
    size_t global_count;
    size_t local_idx;
    size_t local_count;
} atl_proc_coord_t;

typedef struct {
    uint64_t tag;
    size_t remote_proc_idx;
    void* internal[ATL_REQ_SIZE];
} atl_req_t __attribute__((aligned(ATL_CACHELINE_LEN)));

typedef struct {
    const char* name;
    atl_status_t (
        *init)(int* argc, char*** argv, atl_attr_t* attr, atl_ctx_t** ctx, const char* main_addr);
    atl_status_t (*main_addr_reserv)(char* main_addr);
} atl_transport_t;

typedef struct {
    atl_status_t (*finalize)(atl_ctx_t* ctx);
} atl_ops_t;

typedef struct {
    atl_status_t (*mr_reg)(atl_ctx_t* ctx, const void* buf, size_t len, atl_mr_t** mr);
    atl_status_t (*mr_dereg)(atl_ctx_t* ctx, atl_mr_t* mr);
} atl_mr_ops_t;

struct atl_ctx {
    atl_ops_t* ops;
    atl_mr_ops_t* mr_ops;
    atl_proc_coord_t coord;

    size_t ep_count;
    atl_ep_t** eps;

    int is_resize_enabled;
};

/*
   name convention
   len - for bytes
   count - for iov and for dtype-arrays like in reduce/allreduce
*/

typedef struct {
    atl_status_t (*send)(atl_ep_t* ep,
                         const void* buf,
                         size_t len,
                         size_t dst_proc_idx,
                         uint64_t tag,
                         atl_req_t* req);
    atl_status_t (*recv)(atl_ep_t* ep,
                         void* buf,
                         size_t len,
                         size_t src_proc_idx,
                         uint64_t tag,
                         atl_req_t* req);
    atl_status_t (
        *probe)(atl_ep_t* ep, size_t src_proc_idx, uint64_t tag, int* found, size_t* recv_len);
} atl_p2p_ops_t;

typedef struct {
    /* order convention - keep alphabetical order */
    atl_status_t (*allgatherv)(atl_ep_t* ep,
                               const void* send_buf,
                               size_t send_len,
                               void* recv_buf,
                               const int* recv_lens,
                               const int* offsets,
                               atl_req_t* req);
    atl_status_t (*allreduce)(atl_ep_t* ep,
                              const void* send_buf,
                              void* recv_buf,
                              size_t count,
                              atl_datatype_t dtype,
                              atl_reduction_t op,
                              atl_req_t* req);
    atl_status_t (
        *alltoall)(atl_ep_t* ep, const void* send_buf, void* recv_buf, size_t len, atl_req_t* req);
    atl_status_t (*alltoallv)(atl_ep_t* ep,
                              const void* send_buf,
                              const int* send_lens,
                              const int* send_offsets,
                              void* recv_buf,
                              const int* recv_lens,
                              const int* recv_offsets,
                              atl_req_t* req);
    atl_status_t (*barrier)(atl_ep_t* ep, atl_req_t* req);
    atl_status_t (*bcast)(atl_ep_t* ep, void* buf, size_t len, size_t root, atl_req_t* req);
    atl_status_t (*reduce)(atl_ep_t* ep,
                           const void* send_buf,
                           void* recv_buf,
                           size_t count,
                           size_t root,
                           atl_datatype_t dtype,
                           atl_reduction_t op,
                           atl_req_t* req);
} atl_coll_ops_t;

typedef struct {
    atl_status_t (*read)(atl_ep_t* ep,
                         void* buf,
                         size_t len,
                         atl_mr_t* mr,
                         uint64_t addr,
                         uintptr_t remote_key,
                         size_t dst_proc_idx,
                         atl_req_t* req);
    atl_status_t (*write)(atl_ep_t* ep,
                          const void* buf,
                          size_t len,
                          atl_mr_t* mr,
                          uint64_t addr,
                          uintptr_t remote_key,
                          size_t dst_proc_idx,
                          atl_req_t* req);
} atl_rma_ops_t;

typedef struct {
    atl_status_t (*wait)(atl_ep_t* ep, atl_req_t* req);
    atl_status_t (*wait_all)(atl_ep_t* ep, atl_req_t* reqs, size_t count);
    atl_status_t (*cancel)(atl_ep_t* ep, atl_req_t* req);
    atl_status_t (*poll)(atl_ep_t* ep);
    atl_status_t (*check)(atl_ep_t* ep, int* is_completed, atl_req_t* req);
} atl_comp_ops_t;

struct atl_ep {
    size_t idx;
    atl_ctx_t* ctx;
    atl_p2p_ops_t* p2p_ops;
    atl_coll_ops_t* coll_ops;
    atl_rma_ops_t* rma_ops;
    atl_comp_ops_t* comp_ops;
};