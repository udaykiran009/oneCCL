#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "atl.h"
#include <unistd.h>
#include <sys/syscall.h>

#include <mpi.h>

#include <inttypes.h>

#ifndef gettid
#define gettid() syscall(SYS_gettid)
#endif

#define COMM_IDX_MAX_STR_LEN 4
#define COMM_IDX_KEY         "comm_idx"

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

#ifdef ENABLE_DEBUG
#define ATL_MPI_DEBUG_PRINT(s, ...) ATL_MPI_PRINT(s, ##__VA_ARGS__)
#else
#define ATL_MPI_DEBUG_PRINT(s, ...)
#endif

#define ATL_MPI_PM_KEY "atl-mpi"

#define RET2ATL(ret) (ret != MPI_SUCCESS) ? atl_status_failure : atl_status_success

static const char *atl_mpi_name = "MPI";

typedef struct atl_mpi_context {
    atl_desc_t atl_desc;
    size_t proc_idx;
    size_t proc_count;
    size_t comm_ref_count;
} atl_mpi_context_t;

typedef struct atl_mpi_comm_context {
    atl_comm_t atl_comm;
    size_t idx;
    MPI_Comm mpi_comm;
} atl_mpi_comm_context_t;

typedef enum atl_mpi_comp_state {
    ATL_MPI_COMP_POSTED,
    ATL_MPI_COMP_PROBE_COMPLETED,
    ATL_MPI_COMP_COMPLETED
} atl_mpi_comp_state_t;

typedef struct atl_mpi_req {
    MPI_Request mpi_context;
    atl_mpi_comp_state_t comp_state;
} atl_mpi_req_t;

static MPI_Datatype
atl2mpi_dtype(atl_datatype_t dtype)
{
    switch(dtype)
    {
        case atl_dtype_char:
            return MPI_CHAR;
        case atl_dtype_int:
            return MPI_INT;
        case atl_dtype_float:
            return MPI_FLOAT;
        case atl_dtype_double:
            return MPI_DOUBLE;
        default:
            printf("Unknown datatype: %d\n", dtype);
            exit(1);
    }
}

static MPI_Op
atl2mpi_op(atl_reduction_t rtype)
{
    switch(rtype)
    {
        case atl_reduction_sum:
            return MPI_SUM;
        case atl_reduction_prod:
            return MPI_PROD;
        case atl_reduction_min:
            return MPI_MIN;
        case atl_reduction_max:
            return MPI_MAX;
        default:
            printf("Unknown reduction type: %d\n", rtype);
            exit(1);
    }
}

static atl_status_t atl_mpi_finalize(atl_desc_t *atl_desc, atl_comm_t **atl_comms)
{
    int ret;
    int i;
    atl_mpi_context_t *atl_mpi_context =
        container_of(atl_desc, atl_mpi_context_t, atl_desc);

    for (i = 0; i < atl_mpi_context->comm_ref_count; i++)
    {
        atl_mpi_comm_context_t *comm_context =
            container_of(atl_comms[i], atl_mpi_comm_context_t, atl_comm);
        MPI_Comm_free(&comm_context->mpi_comm);
    }

    free(atl_comms);
    free(atl_mpi_context);

    ret = MPI_Finalize();

    return RET2ATL(ret);
}


static inline atl_status_t atl_mpi_comm_poll(atl_comm_t *comm)
{
    return atl_status_success;
}

/* Non-blocking I/O vector pt2pt ops */
static atl_status_t
atl_mpi_comm_sendv(atl_comm_t *comm, const struct iovec *iov, size_t count,
                   size_t dest_proc_idx, uint64_t tag, atl_req_t *req)
{
    return atl_status_unsupported;
}

static atl_status_t
atl_mpi_comm_recvv(atl_comm_t *comm, struct iovec *iov, size_t count,
                   size_t src_proc_idx, uint64_t tag, atl_req_t *req)
{
    return atl_status_unsupported;
}

/* Non-blocking pt2pt ops */
static atl_status_t
atl_mpi_comm_send(atl_comm_t *comm, const void *buf, size_t cnt,
                  size_t dest_proc_idx, uint64_t tag, atl_req_t *req)
{
    atl_mpi_comm_context_t *comm_context =
            container_of(comm, atl_mpi_comm_context_t, atl_comm);
    atl_mpi_req_t *mpi_req = ((atl_mpi_req_t *)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    int ret = MPI_Isend(buf, cnt, MPI_CHAR, dest_proc_idx,
                        (int)tag, comm_context->mpi_comm, &mpi_req->mpi_context);
    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_comm_recv(atl_comm_t *comm, void *buf, size_t cnt,
                  size_t src_proc_idx, uint64_t tag, atl_req_t *req)
{
    atl_mpi_comm_context_t *comm_context =
        container_of(comm, atl_mpi_comm_context_t, atl_comm);
    atl_mpi_req_t *mpi_req = ((atl_mpi_req_t *)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    int ret = MPI_Irecv(buf, cnt, MPI_CHAR, src_proc_idx,
                        (int)tag, comm_context->mpi_comm, &mpi_req->mpi_context);
    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_comm_allreduce(atl_comm_t *comm, const void *send_buf, void *recv_buf, size_t cnt,
                       atl_datatype_t dtype, atl_reduction_t op, atl_req_t *req)
{
    atl_mpi_comm_context_t *comm_context =
        container_of(comm, atl_mpi_comm_context_t, atl_comm);
    atl_mpi_req_t *mpi_req = ((atl_mpi_req_t *)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    MPI_Datatype mpi_dtype = atl2mpi_dtype(dtype);
    MPI_Op mpi_op = atl2mpi_op(op);
    int ret = MPI_Iallreduce((send_buf == recv_buf) ? MPI_IN_PLACE : send_buf,
                             recv_buf, cnt, mpi_dtype, mpi_op,
                             comm_context->mpi_comm, &mpi_req->mpi_context);

#ifdef ENABLE_DEBUG
    MPI_Info info_out;
    char buf[MPI_MAX_INFO_VAL];
    int flag;
    MPI_Comm_get_info(comm_context->mpi_comm, &info_out);
    MPI_Info_get(info_out, COMM_IDX_KEY, MPI_MAX_INFO_VAL, buf, &flag);
    if (!flag) {
        printf("unexpected key %s", COMM_IDX_KEY);
        return atl_status_failure;
    }
    else {
        //ATL_MPI_DEBUG_PRINT("count %zu, comm_key %s", cnt, buf);
    }
    MPI_Info_free(&info_out);
#endif

    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_comm_reduce(atl_comm_t *comm, const void *send_buf, void *recv_buf, size_t cnt, size_t root,
                    atl_datatype_t dtype, atl_reduction_t op, atl_req_t *req)
{
    atl_desc_t *atl_desc = comm->atl_desc;
    atl_mpi_context_t *atl_mpi_context =
        container_of(atl_desc, atl_mpi_context_t, atl_desc);
    atl_mpi_comm_context_t *comm_context =
        container_of(comm, atl_mpi_comm_context_t, atl_comm);
    atl_mpi_req_t *mpi_req = ((atl_mpi_req_t *)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    MPI_Datatype mpi_dtype = atl2mpi_dtype(dtype);
    MPI_Op mpi_op = atl2mpi_op(op);
    int ret = MPI_Ireduce(((send_buf == recv_buf) && (root == atl_mpi_context->proc_idx)) ? MPI_IN_PLACE : send_buf,
                          recv_buf, cnt, mpi_dtype, mpi_op, root,
                          comm_context->mpi_comm, &mpi_req->mpi_context);
    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_comm_barrier(atl_comm_t *comm, atl_req_t *req)
{
    atl_mpi_comm_context_t *comm_context =
        container_of(comm, atl_mpi_comm_context_t, atl_comm);
    atl_mpi_req_t *mpi_req = ((atl_mpi_req_t *)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    int ret = MPI_Ibarrier(comm_context->mpi_comm, &mpi_req->mpi_context);
    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_comm_allgatherv(atl_comm_t *comm, const void *send_buf, size_t send_cnt,
                        void *recv_buf, int recv_cnts[], int displs[], atl_req_t *req)
{
    atl_mpi_comm_context_t *comm_context =
        container_of(comm, atl_mpi_comm_context_t, atl_comm);
    atl_mpi_req_t *mpi_req = ((atl_mpi_req_t *)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    int ret = MPI_Iallgatherv(send_buf, send_cnt, MPI_CHAR, recv_buf, recv_cnts, displs,
                              MPI_CHAR, comm_context->mpi_comm, &mpi_req->mpi_context);
    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_comm_bcast(atl_comm_t *comm, void *buf, size_t cnt, size_t root,
                   atl_req_t *req)
{
    atl_mpi_comm_context_t *comm_context =
        container_of(comm, atl_mpi_comm_context_t, atl_comm);
    atl_mpi_req_t *mpi_req = ((atl_mpi_req_t *)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_POSTED;

    int ret = MPI_Ibcast(buf, cnt, MPI_CHAR, root,
                             comm_context->mpi_comm, &mpi_req->mpi_context);
    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_comm_read(atl_comm_t *comm, void *buf, size_t cnt, atl_mr_t *atl_mr,
                  uint64_t addr, uintptr_t r_key, size_t dest_proc_idx, atl_req_t *req)
{
    return atl_status_unsupported;
}

static atl_status_t
atl_mpi_comm_write(atl_comm_t *comm, const void *buf, size_t cnt, atl_mr_t *atl_mr,
                   uint64_t addr, uintptr_t r_key, size_t dest_proc_idx, atl_req_t *req)
{
    return atl_status_unsupported;
}

static atl_status_t
atl_mpi_comm_probe(atl_comm_t *comm, size_t src_proc_idx, uint64_t tag, atl_req_t *req)
{
    MPI_Status status;
    atl_mpi_comm_context_t *comm_context =
        container_of(comm, atl_mpi_comm_context_t, atl_comm);
    int ret = MPI_Probe(src_proc_idx, (int)tag, comm_context->mpi_comm, &status);
    req->recv_len = 0;
    MPI_Get_count(&status, MPI_BYTE, (int*)&req->recv_len);
    atl_mpi_req_t *mpi_req = ((atl_mpi_req_t *)req->internal);
    mpi_req->comp_state = ATL_MPI_COMP_PROBE_COMPLETED;
    return RET2ATL(ret);
}

static atl_status_t atl_mpi_comm_wait(atl_comm_t *comm, atl_req_t *req)
{
    atl_status_t ret;
    MPI_Status status;
    atl_mpi_req_t *mpi_req = ((atl_mpi_req_t *)req->internal);
    ret = MPI_Wait(&mpi_req->mpi_context, &status);
    mpi_req->comp_state = ATL_MPI_COMP_COMPLETED;
    return RET2ATL(ret);
}

static atl_status_t
atl_mpi_comm_wait_all(atl_comm_t *comm, atl_req_t *reqs, size_t count)
{
    return atl_status_unsupported;
}

static atl_status_t
atl_mpi_comm_check(atl_comm_t *comm, int *status, atl_req_t *req)
{
    atl_mpi_req_t *mpi_req = ((atl_mpi_req_t *)req->internal);
    if (mpi_req->comp_state == ATL_MPI_COMP_PROBE_COMPLETED)
    {
        *status = 1;
        return atl_status_success;
    }

    MPI_Status mpi_status;
    int ret;
    ret = MPI_Test(&mpi_req->mpi_context, status, &mpi_status);

    if (*status)
    {
        mpi_req->comp_state = ATL_MPI_COMP_COMPLETED;
    }
    return RET2ATL(ret);
}

static atl_coll_ops_t atl_mpi_comm_coll_ops = {
    .allreduce = atl_mpi_comm_allreduce,
    .reduce = atl_mpi_comm_reduce,
    .allgatherv = atl_mpi_comm_allgatherv,
    .bcast = atl_mpi_comm_bcast,
    .barrier = atl_mpi_comm_barrier,
};

static atl_pt2pt_ops_t atl_mpi_comm_pt2pt_ops = {
    .sendv = atl_mpi_comm_sendv,
    .recvv = atl_mpi_comm_recvv,
    .send = atl_mpi_comm_send,
    .recv = atl_mpi_comm_recv,
    .probe = atl_mpi_comm_probe,
};

static atl_rma_ops_t atl_mpi_comm_rma_ops = {
    .read = atl_mpi_comm_read,
    .write = atl_mpi_comm_write,
};

static atl_comp_ops_t atl_mpi_comm_comp_ops = {
    .wait = atl_mpi_comm_wait,
    .wait_all = atl_mpi_comm_wait_all,
    .poll = atl_mpi_comm_poll,
    .check = atl_mpi_comm_check
};

static void atl_mpi_proc_idx(atl_desc_t *atl_desc, size_t *proc_idx)
{
    atl_mpi_context_t *atl_mpi_context =
        container_of(atl_desc, atl_mpi_context_t, atl_desc);
    *proc_idx = atl_mpi_context->proc_idx;
}

static void atl_mpi_proc_count(atl_desc_t *atl_desc, size_t *proc_count)
{
    atl_mpi_context_t *atl_mpi_context =
        container_of(atl_desc, atl_mpi_context_t, atl_desc);
    *proc_count = atl_mpi_context->proc_count;
}

static atl_status_t
atl_mpi_update(size_t *proc_idx, size_t *proc_count, atl_desc_t *atl_desc)
{
    return atl_status_unsupported;
}

static atl_status_t atl_mpi_mr_reg(atl_desc_t *atl_desc, const void *buf, size_t cnt,
                                   atl_mr_t **atl_mr)
{
    return atl_status_unsupported;
}

static atl_status_t atl_mpi_mr_dereg(atl_desc_t *atl_desc, atl_mr_t *atl_mr)
{
    return atl_status_unsupported;
}

static atl_status_t atl_mpi_set_framework_function(update_checker_f user_checker)
{
    return atl_status_unsupported;
}

atl_ops_t atl_mpi_ops = {
    .proc_idx               = atl_mpi_proc_idx,
    .proc_count             = atl_mpi_proc_count,
    .finalize               = atl_mpi_finalize,
    .update                 = atl_mpi_update,
    .set_framework_function = atl_mpi_set_framework_function,
};

static atl_mr_ops_t atl_mpi_mr_ops = {
    .mr_reg = atl_mpi_mr_reg,
    .mr_dereg = atl_mpi_mr_dereg,
};

static atl_status_t
atl_mpi_comm_init(size_t index, atl_comm_t **comm)
{
    int ret;
    atl_mpi_comm_context_t *atl_mpi_comm_context;

    atl_mpi_comm_context = calloc(1, sizeof(*atl_mpi_comm_context));
    if (!atl_mpi_comm_context)
        return atl_status_failure;

    ret = MPI_Comm_dup(MPI_COMM_WORLD, &atl_mpi_comm_context->mpi_comm);
    if (ret)
        goto err_comm_dup;

    MPI_Info info;
    MPI_Info_create(&info);
    char comm_idx_str[COMM_IDX_MAX_STR_LEN] = { 0 };
    snprintf(comm_idx_str, COMM_IDX_MAX_STR_LEN, "%zu", index);
    MPI_Info_set(info, COMM_IDX_KEY, comm_idx_str);
    MPI_Comm_set_info(atl_mpi_comm_context->mpi_comm, info);
    MPI_Info_free(&info);

    ATL_MPI_DEBUG_PRINT("atl_mpi_comm_init: idx %zu, comm_id_str %s", index, comm_idx_str);

    atl_mpi_comm_context->idx = index;
    *comm = &atl_mpi_comm_context->atl_comm;
    (*comm)->coll_ops = &atl_mpi_comm_coll_ops;
    (*comm)->pt2pt_ops = &atl_mpi_comm_pt2pt_ops;
    (*comm)->rma_ops = &atl_mpi_comm_rma_ops;
    (*comm)->comp_ops = &atl_mpi_comm_comp_ops;

    return atl_status_success;
err_comm_dup:
    free(atl_mpi_comm_context);
    return RET2ATL(ret);
}

atl_status_t atl_mpi_init(int *argc, char ***argv, size_t *proc_idx, size_t *proc_count,
                          atl_attr_t *attr, atl_comm_t ***atl_comms, atl_desc_t **atl_desc)
{
    assert(sizeof(atl_mpi_req_t) <= sizeof(atl_req_t) - offsetof(atl_req_t, internal));

    int ret;
    size_t i;
    atl_mpi_context_t *atl_mpi_context;
    int required_thread_level = MPI_THREAD_MULTIPLE, provided_thread_level;

    atl_mpi_context = calloc(1, sizeof(*atl_mpi_context));
    if (!atl_mpi_context)
        return atl_status_failure;

    if (attr->comm_count > 1)
    {
        char comm_count_str[COMM_IDX_MAX_STR_LEN] = { 0 };
        snprintf(comm_count_str, COMM_IDX_MAX_STR_LEN, "%zu", attr->comm_count);

        setenv("I_MPI_THREAD_SPLIT", "1", 0);
        setenv("I_MPI_THREAD_RUNTIME", "generic", 0);
        setenv("I_MPI_THREAD_MAX", comm_count_str, 0);
        setenv("I_MPI_THREAD_ID_KEY", COMM_IDX_KEY, 0);
        setenv("I_MPI_THREAD_LOCK_LEVEL", "vci", 0);

        ret = MPI_Init_thread(argc, argv, required_thread_level, &provided_thread_level);
        if (provided_thread_level < required_thread_level)
            goto err_init;
    }
    else
        ret = MPI_Init(argc, argv);

    if (ret)
        goto err_init;

    MPI_Comm_rank(MPI_COMM_WORLD, (int*)proc_idx);
    MPI_Comm_size(MPI_COMM_WORLD, (int*)proc_count);

    atl_mpi_context->proc_count = *proc_count;
    atl_mpi_context->proc_idx = *proc_idx;
    atl_mpi_context->comm_ref_count = 0;

    *atl_comms = calloc(1, sizeof(atl_comm_t**) * attr->comm_count);
    if (!*atl_comms)
        goto err_comm;

    for (i = 0; i < attr->comm_count; i++)
    {
        ret = atl_mpi_comm_init(i, &(*atl_comms)[i]);
        if (ret)
            goto err_comm_dup;
        atl_mpi_context->comm_ref_count++;
    }


    atl_mpi_context->atl_desc.ops = &atl_mpi_ops;
    atl_mpi_context->atl_desc.mr_ops = &atl_mpi_mr_ops;
    *atl_desc = &atl_mpi_context->atl_desc;

    attr->is_tagged_coll_enabled = 0;
    attr->tag_bits = 32;
    attr->max_tag = 0;
    int flag;
    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, &(attr->max_tag), &flag);

    return atl_status_success;

err_comm_dup:
    for (i = 0; i < atl_mpi_context->comm_ref_count; i++)
    {
        atl_mpi_comm_context_t *comm_context =
            container_of((*atl_comms)[i], atl_mpi_comm_context_t, atl_comm);
        MPI_Comm_free(&comm_context->mpi_comm);
    }
    free(*atl_comms);
err_comm:
    MPI_Finalize();
err_init:
    free(atl_mpi_context);
    return atl_status_failure;
}

ATL_MPI_INI
{
    atl_transport->name = atl_mpi_name;
    atl_transport->init = atl_mpi_init;

    return atl_status_success;
}
