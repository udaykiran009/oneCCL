#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "atl.h"

const int optimized_impi_versions[] = { 2019, 2020, 2021 };
int is_external_init = 0;

#define ATL_MPI_PM_KEY              "atl-mpi"
#define EP_IDX_MAX_STR_LEN           4
#define EP_IDX_KEY                   "ep_idx"
#define OPTIMIZED_MPI_VERSION_PREFIX "Intel(R) MPI Library"

#define ATL_MPI_PRINT(s, ...)                             \
    do {                                                  \
        pid_t tid = gettid();                             \
        char hoststr[32];                                 \
        gethostname(hoststr, sizeof(hoststr));            \
        fprintf(stdout, "(%d): %s: @ %s:%d:%s() " s "\n", \
                tid, hoststr,                             \
                __FILE__, __LINE__,                       \
                __func__, ##__VA_ARGS__);                 \
        fflush(stdout);                                   \
    } while (0)

#define ATL_MPI_ASSERT(cond, ...)                 \
    do {                                          \
        if (!(cond))                              \
        {                                         \
            ATL_MPI_PRINT("ASSERT failed, cond: " \
                          #cond " " __VA_ARGS__); \
            exit(0);                              \
        }                                         \
    } while(0)

#ifdef ENABLE_DEBUG
#define ATL_MPI_DEBUG_PRINT(s, ...) ATL_MPI_PRINT(s, ##__VA_ARGS__)
#else
#define ATL_MPI_DEBUG_PRINT(s, ...)
#endif

#define RET2ATL(ret) (ret != MPI_SUCCESS) ? ATL_STATUS_FAILURE : ATL_STATUS_SUCCESS

typedef struct
{
    int use_optimized_mpi;
} atl_mpi_global_data_t;

static atl_mpi_global_data_t global_data;

typedef struct
{
    atl_ctx_t ctx;
} atl_mpi_ctx_t;

typedef struct
{
    atl_ep_t ep;
    MPI_Comm mpi_comm;
} atl_mpi_ep_t;

typedef enum
{
    ATL_MPI_COMP_POSTED,
    ATL_MPI_COMP_COMPLETED
} atl_mpi_comp_state_t;

typedef struct
{
    MPI_Request native_req;
    atl_mpi_comp_state_t comp_state;
} atl_mpi_req_t;

static MPI_Datatype
atl2mpi_dtype(atl_datatype_t dtype)
{
    switch (dtype)
    {
        case ATL_DTYPE_CHAR:
            return MPI_CHAR;
        case ATL_DTYPE_INT:
            return MPI_INT;
        case ATL_DTYPE_FLOAT:
            return MPI_FLOAT;
        case ATL_DTYPE_DOUBLE:
            return MPI_DOUBLE;
        case ATL_DTYPE_INT64:
            return MPI_LONG_LONG;
        case ATL_DTYPE_UINT64:
            return MPI_UNSIGNED_LONG_LONG;
        default:
            printf("Unknown datatype: %d\n", dtype);
            exit(1);
    }
}

static MPI_Op
atl2mpi_op(atl_reduction_t rtype)
{
    switch (rtype)
    {
        case ATL_REDUCTION_SUM:
            return MPI_SUM;
        case ATL_REDUCTION_PROD:
            return MPI_PROD;
        case ATL_REDUCTION_MIN:
            return MPI_MIN;
        case ATL_REDUCTION_MAX:
            return MPI_MAX;
        default:
            printf("Unknown reduction type: %d\n", rtype);
            exit(1);
    }
}

int atl_mpi_use_optimized_mpi(const atl_attr_t* attr)
{
    int i, use_optimized_mpi = 0;
    char mpi_version[MPI_MAX_LIBRARY_VERSION_STRING];
    int mpi_version_len;
    char* use_opt_env = NULL;

    if (attr->ep_count == 1)
    {
        ATL_MPI_DEBUG_PRINT("set use_optimized_mpi = 0 because single ATL EP is requested");
        use_optimized_mpi = 0;
    }
    else if (attr->ep_count > 1)
    {
        MPI_Get_library_version(mpi_version, &mpi_version_len);
        ATL_MPI_DEBUG_PRINT("initial use_optimized_mpi = %d", use_optimized_mpi);
        ATL_MPI_DEBUG_PRINT("MPI version %s", mpi_version);

        if (strncmp(mpi_version, OPTIMIZED_MPI_VERSION_PREFIX, strlen(OPTIMIZED_MPI_VERSION_PREFIX)) == 0)
        {
            int impi_version = atoi(mpi_version + strlen(OPTIMIZED_MPI_VERSION_PREFIX));
            ATL_MPI_DEBUG_PRINT("IMPI version %d", impi_version);
            for (i = 0; i < SIZEOFARR(optimized_impi_versions); i++)
            {
                if (impi_version == optimized_impi_versions[i])
                {
                    ATL_MPI_DEBUG_PRINT("set use_optimized_mpi = 1 because IMPI version matches with expected one");
                    use_optimized_mpi = 1;
                    break;
                }
            }
        }
    }

    if ((use_opt_env = getenv("CCL_ATL_MPI_OPT")) != NULL)
    {
        use_optimized_mpi = atoi(use_opt_env);
        ATL_MPI_DEBUG_PRINT("set use_optimized_mpi = %d because optimized MPI environment is requested explicitly",
            use_optimized_mpi);
    }

    ATL_MPI_DEBUG_PRINT("final use_optimized_mpi = %d", use_optimized_mpi);

    return use_optimized_mpi;
}

atl_status_t atl_mpi_set_optimized_mpi_environment(const atl_attr_t* attr)
{
    char ep_count_str[EP_IDX_MAX_STR_LEN] = { 0 };
    snprintf(ep_count_str, EP_IDX_MAX_STR_LEN, "%zu", attr->ep_count);

    setenv("I_MPI_THREAD_SPLIT", "1", 0);
    setenv("I_MPI_THREAD_RUNTIME", "generic", 0);
    setenv("I_MPI_THREAD_MAX", ep_count_str, 0);
    setenv("I_MPI_THREAD_ID_KEY", EP_IDX_KEY, 0);
    setenv("I_MPI_THREAD_LOCK_LEVEL", "vci", 0);

    return ATL_STATUS_SUCCESS;
}

static atl_status_t
atl_mpi_finalize(atl_ctx_t* ctx)
{
    int ret = MPI_SUCCESS;

    atl_mpi_ctx_t* mpi_ctx =
        container_of(ctx, atl_mpi_ctx_t, ctx);

    atl_ep_t** eps = ctx->eps;

    int is_mpi_finalized = 0;
    MPI_Finalized(&is_mpi_finalized);

    if (!is_mpi_finalized)
    {
        for (int i = 0; i < ctx->ep_count; i++)
        {
            atl_mpi_ep_t* mpi_ep =
                container_of(eps[i], atl_mpi_ep_t, ep);

            if (mpi_ep)
            {
                MPI_Comm_free(&mpi_ep->mpi_comm);
                free(mpi_ep);
            }
        }

        if (!is_external_init)
            ret = MPI_Finalize();
        else
            ATL_MPI_DEBUG_PRINT("MPI_Init has been called externally, skip MPI_Finalize");
    }
    else
        ATL_MPI_DEBUG_PRINT("MPI_Finalize has been already called");
        

    free(eps);
    free(mpi_ctx);

    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_update(atl_ctx_t* ctx)
{
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t
atl_mpi_wait_notification(atl_ctx_t* ctx)
{
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t
atl_mpi_set_resize_function(atl_resize_fn_t fn)
{
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t
atl_mpi_mr_reg(atl_ctx_t* ctx, const void* buf, size_t len, atl_mr_t** mr)
{
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t
atl_mpi_mr_dereg(atl_ctx_t* ctx, atl_mr_t* mr)
{
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t
atl_mpi_ep_send(atl_ep_t* ep, const void* buf, size_t len,
                size_t dest_proc_idx, uint64_t tag, atl_req_t* req)
{
    atl_mpi_ep_t* mpi_ep =
            container_of(ep, atl_mpi_ep_t, ep);
    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    int ret = MPI_Isend(buf, len, MPI_CHAR, dest_proc_idx,
                        (int)tag, mpi_ep->mpi_comm, &mpi_req->native_req);
    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_ep_recv(atl_ep_t* ep, void* buf, size_t len,
                size_t src_proc_idx, uint64_t tag, atl_req_t* req)
{
    atl_mpi_ep_t* mpi_ep =
        container_of(ep, atl_mpi_ep_t, ep);
    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    int ret = MPI_Irecv(buf, len, MPI_CHAR, src_proc_idx,
                        (int)tag, mpi_ep->mpi_comm, &mpi_req->native_req);
    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_ep_probe(atl_ep_t* ep, size_t src_proc_idx,
                 uint64_t tag, int* found, size_t *recv_len)
{
    atl_mpi_ep_t* mpi_ep =
        container_of(ep, atl_mpi_ep_t, ep);

    int flag = 0, len = 0, ret;
    MPI_Status status;

    ret = MPI_Iprobe(src_proc_idx, tag, mpi_ep->mpi_comm, &flag, &status);
    if (flag)
    {
        MPI_Get_count(&status, MPI_BYTE, &len);
    }

    if (found) *found = flag;
    if (recv_len) *recv_len = len;

    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_ep_allgatherv(atl_ep_t* ep, const void* send_buf, size_t send_len,
                      void* recv_buf, const int recv_lens[], int displs[], atl_req_t* req)
{
    atl_mpi_ep_t* mpi_ep =
        container_of(ep, atl_mpi_ep_t, ep);

    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);
    int ret = MPI_SUCCESS;

    ret = MPI_Iallgatherv((send_buf == recv_buf) ? MPI_IN_PLACE : send_buf, send_len, MPI_CHAR,
                          recv_buf, recv_lens, displs, MPI_CHAR,
                          mpi_ep->mpi_comm, &mpi_req->native_req);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;
    
    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_ep_allreduce(atl_ep_t* ep, const void* send_buf, void* recv_buf, size_t count,
                     atl_datatype_t dtype, atl_reduction_t op, atl_req_t* req)
{
    atl_mpi_ep_t* mpi_ep =
        container_of(ep, atl_mpi_ep_t, ep);

    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    MPI_Datatype mpi_dtype = atl2mpi_dtype(dtype);
    MPI_Op mpi_op = atl2mpi_op(op);
    int ret = MPI_Iallreduce((send_buf == recv_buf) ? MPI_IN_PLACE : send_buf,
                             recv_buf, count, mpi_dtype, mpi_op,
                             mpi_ep->mpi_comm, &mpi_req->native_req);

#ifdef ENABLE_DEBUG
    if (global_data.use_optimized_mpi)
    {
        MPI_Info info_out;
        char buf[MPI_MAX_INFO_VAL];
        int flag;
        MPI_Comm_get_info(mpi_ep->mpi_comm, &info_out);
        MPI_Info_get(info_out, EP_IDX_KEY, MPI_MAX_INFO_VAL, buf, &flag);
        if (!flag)
        {
            printf("unexpected key %s\n", EP_IDX_KEY);
            return ATL_STATUS_FAILURE;
        }
        else
        {
            //ATL_MPI_DEBUG_PRINT("allreduce: count %zu, comm_key %s, comm %p", count, buf, comm);
        }
        MPI_Info_free(&info_out);
    }
#endif

    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_ep_alltoall(atl_ep_t* ep, const void* send_buf, void* recv_buf,
                    size_t len, atl_req_t* req)
{
    atl_mpi_ep_t* mpi_ep =
        container_of(ep, atl_mpi_ep_t, ep);

    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);
    int ret = MPI_SUCCESS;

    ret = MPI_Ialltoall((send_buf == recv_buf) ? MPI_IN_PLACE : send_buf, len,  MPI_CHAR,
                        recv_buf, len, MPI_CHAR,
                        mpi_ep->mpi_comm, &mpi_req->native_req);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_ep_barrier(atl_ep_t* ep, atl_req_t* req)
{
    atl_mpi_ep_t* mpi_ep =
        container_of(ep, atl_mpi_ep_t, ep);

    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    int ret = MPI_Ibarrier(mpi_ep->mpi_comm, &mpi_req->native_req);

    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_ep_bcast(atl_ep_t* ep, void* buf, size_t len, size_t root,
                   atl_req_t* req)
{
    atl_mpi_ep_t* mpi_ep =
        container_of(ep, atl_mpi_ep_t, ep);

    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    int ret = MPI_Ibcast(buf, len, MPI_CHAR, root,
                         mpi_ep->mpi_comm, &mpi_req->native_req);
    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_ep_reduce(atl_ep_t* ep, const void* send_buf, void* recv_buf, size_t count, size_t root,
                  atl_datatype_t dtype, atl_reduction_t op, atl_req_t* req)
{
    size_t my_proc_idx = ep->ctx->coord.global_idx;

    atl_mpi_ep_t* mpi_ep =
        container_of(ep, atl_mpi_ep_t, ep);

    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    MPI_Datatype mpi_dtype = atl2mpi_dtype(dtype);
    MPI_Op mpi_op = atl2mpi_op(op);
    int ret = MPI_Ireduce(((send_buf == recv_buf) && (root == my_proc_idx)) ? MPI_IN_PLACE : send_buf,
                          recv_buf, count, mpi_dtype, mpi_op, root,
                          mpi_ep->mpi_comm, &mpi_req->native_req);

    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_ep_read(atl_ep_t* ep, void* buf, size_t len, atl_mr_t* mr,
                uint64_t addr, uintptr_t r_key, size_t dest_proc_idx, atl_req_t* req)
{
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t
atl_mpi_ep_write(atl_ep_t* ep, const void* buf, size_t len, atl_mr_t* mr,
                 uint64_t addr, uintptr_t r_key, size_t dest_proc_idx, atl_req_t* req)
{
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t
atl_mpi_ep_wait(atl_ep_t* ep, atl_req_t* req)
{
    atl_status_t ret;
    MPI_Status status;
    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);
    ret = MPI_Wait(&mpi_req->native_req, &status);
    mpi_req->comp_state = ATL_MPI_COMP_COMPLETED;
    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_ep_wait_all(atl_ep_t* ep, atl_req_t* reqs, size_t count)
{
    return ATL_STATUS_UNSUPPORTED;
}

static inline atl_status_t
atl_mpi_ep_poll(atl_ep_t* ep)
{
    return ATL_STATUS_SUCCESS;
}

static atl_status_t
atl_mpi_ep_check(atl_ep_t* ep, int* status, atl_req_t* req)
{
    atl_mpi_req_t* mpi_req = ((atl_mpi_req_t*)req->internal);
    if (mpi_req->comp_state == ATL_MPI_COMP_COMPLETED)
    {
        *status = 1;
        return ATL_STATUS_SUCCESS;
    }

    int flag = 0;
    MPI_Status mpi_status;
    int ret = MPI_Test(&mpi_req->native_req, &flag, &mpi_status);
    if (flag)
    {
        mpi_req->comp_state = ATL_MPI_COMP_COMPLETED;
    }

    if (status) *status = flag;

    return RET2ATL(ret);
}

static atl_ops_t atl_mpi_ops =
{
    .finalize            = atl_mpi_finalize,
    .update              = atl_mpi_update,
    .wait_notification   = atl_mpi_wait_notification,
    .set_resize_function = atl_mpi_set_resize_function,
};

static atl_mr_ops_t atl_mpi_mr_ops =
{
    .mr_reg   = atl_mpi_mr_reg,
    .mr_dereg = atl_mpi_mr_dereg,
};

static atl_p2p_ops_t atl_mpi_ep_p2p_ops =
{
    .send  = atl_mpi_ep_send,
    .recv  = atl_mpi_ep_recv,
    .probe = atl_mpi_ep_probe,
};

static atl_coll_ops_t atl_mpi_ep_coll_ops =
{
    .allgatherv = atl_mpi_ep_allgatherv,
    .allreduce  = atl_mpi_ep_allreduce,
    .alltoall   = atl_mpi_ep_alltoall,
    .barrier    = atl_mpi_ep_barrier,
    .bcast      = atl_mpi_ep_bcast,
    .reduce     = atl_mpi_ep_reduce
};

static atl_rma_ops_t atl_mpi_ep_rma_ops =
{
    .read  = atl_mpi_ep_read,
    .write = atl_mpi_ep_write,
};

static atl_comp_ops_t atl_mpi_ep_comp_ops =
{
    .wait     = atl_mpi_ep_wait,
    .wait_all = atl_mpi_ep_wait_all,
    .poll     = atl_mpi_ep_poll,
    .check    = atl_mpi_ep_check
};

static atl_status_t
atl_mpi_ep_init(atl_mpi_ctx_t* mpi_ctx, size_t idx, atl_ep_t** ep)
{
    int ret;
    atl_mpi_ep_t* mpi_ep = calloc(1, sizeof(atl_mpi_ep_t));
    if (!mpi_ep)
        return ATL_STATUS_FAILURE;

    ret = MPI_Comm_dup(MPI_COMM_WORLD, &mpi_ep->mpi_comm);
    if (ret)
        goto err_ep_dup;

    MPI_Info info;
    MPI_Info_create(&info);
    char ep_idx_str[EP_IDX_MAX_STR_LEN] = { 0 };
    snprintf(ep_idx_str, EP_IDX_MAX_STR_LEN, "%zu", idx);
    MPI_Info_set(info, EP_IDX_KEY, ep_idx_str);
    MPI_Comm_set_info(mpi_ep->mpi_comm, info);
    MPI_Info_free(&info);

    ATL_MPI_DEBUG_PRINT("idx %zu, ep_idx_str %s", idx, ep_idx_str);

    *ep = &mpi_ep->ep;
    (*ep)->idx = idx;
    (*ep)->ctx = &mpi_ctx->ctx;
    (*ep)->p2p_ops = &atl_mpi_ep_p2p_ops;
    (*ep)->coll_ops = &atl_mpi_ep_coll_ops;
    (*ep)->rma_ops = &atl_mpi_ep_rma_ops;
    (*ep)->comp_ops = &atl_mpi_ep_comp_ops;

    return ATL_STATUS_SUCCESS;

err_ep_dup:
    free(mpi_ep);
    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_init(int* argc, char*** argv,
             atl_attr_t* attr,
             atl_ctx_t** out_ctx)
{
    ATL_MPI_ASSERT((sizeof(atl_mpi_req_t) <= sizeof(atl_req_t) - offsetof(atl_req_t, internal)),
                   "unexpected offset: atl_mpi_request size %zu, atl_request size %zu, expected offset %zu",
                   sizeof(atl_mpi_req_t), sizeof(atl_req_t), offsetof(atl_req_t, internal));

    int ret = MPI_SUCCESS;
    size_t i;
    int is_tag_ub_set = 0;
    void* tag_ub_ptr = NULL;
    int required_thread_level = MPI_THREAD_MULTIPLE, provided_thread_level;

    atl_mpi_ctx_t* mpi_ctx = calloc(1, sizeof(atl_mpi_ctx_t));
    if (!mpi_ctx)
        return ATL_STATUS_FAILURE;

    atl_ctx_t* ctx = &(mpi_ctx->ctx);

    global_data.use_optimized_mpi = atl_mpi_use_optimized_mpi(attr);

    if (global_data.use_optimized_mpi)
    {
        atl_mpi_set_optimized_mpi_environment(attr);
    }

    if (attr->enable_shm)
        setenv("I_MPI_FABRICS", "shm:ofi", 0);
    else
        setenv("I_MPI_FABRICS", "ofi", 0);
    
    int is_mpi_inited = 0;
    MPI_Initialized(&is_mpi_inited);
    
    if (!is_mpi_inited)
    {
        ret = MPI_Init_thread(argc, argv, required_thread_level, &provided_thread_level);
        if (provided_thread_level < required_thread_level)
            goto err_init;
    }
    else
    {
        is_external_init = 1;
        ATL_MPI_DEBUG_PRINT("MPI was initialized externaly");
    }  

    if (ret)
        goto err_init;

    atl_proc_coord_t* coord = &(ctx->coord);
    
    MPI_Comm_rank(MPI_COMM_WORLD, (int*)&(coord->global_idx));
    MPI_Comm_size(MPI_COMM_WORLD, (int*)&(coord->global_count));

    MPI_Comm local_comm;
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED,
                        coord->global_count, MPI_INFO_NULL, &local_comm);
    MPI_Comm_rank(local_comm, (int*)&(coord->local_idx));
    MPI_Comm_size(local_comm, (int*)&(coord->local_count));
    MPI_Comm_free(&local_comm);

    ctx->ops = &atl_mpi_ops;
    ctx->mr_ops = &atl_mpi_mr_ops;
    ctx->ep_count = attr->ep_count;
    ctx->eps = calloc(1, sizeof(void*) * attr->ep_count);
    if (!ctx->eps)
        goto err_after_init;
    ctx->is_resize_enabled = 0;

    for (i = 0; i < attr->ep_count; i++)
    {
        ret = atl_mpi_ep_init(mpi_ctx, i, &(ctx->eps[i]));
        if (ret)
            goto err_ep_dup;
    }

    *out_ctx = &mpi_ctx->ctx;

    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB,
                      &tag_ub_ptr, &is_tag_ub_set);

    attr->tag_bits = 32;
    attr->max_tag = (is_tag_ub_set) ? *((int*)tag_ub_ptr) : 0;
    attr->enable_rma = 0;
    attr->max_order_waw_size = 0;

    return ATL_STATUS_SUCCESS;

err_ep_dup:
    for (i = 0; i < attr->ep_count; i++)
    {
        atl_mpi_ep_t* mpi_ep =
            container_of(ctx->eps[i], atl_mpi_ep_t, ep);

        if (ctx->eps[i] && mpi_ep)
            MPI_Comm_free(&mpi_ep->mpi_comm);
    }
    free(ctx->eps);

err_after_init:
    if (!is_external_init)
        MPI_Finalize();

err_init:
    free(mpi_ctx);
    return ATL_STATUS_FAILURE;
}

ATL_MPI_INI
{
    atl_transport->name = "mpi";
    atl_transport->init = atl_mpi_init;
    return ATL_STATUS_SUCCESS;
}
