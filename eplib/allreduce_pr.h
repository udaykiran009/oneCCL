#ifndef ALLREDUCE_PR_H
#define ALLREDUCE_PR_H

#include <mpi.h>
#include <stdio.h>

#define MAX_ALLREDUCE_REQS (512)

typedef enum req_state
{
    REQ_FREE        = 0,
    REQ_IN_PROGRESS = 1,
    REQ_COMPLETED   = 2
} req_state;

typedef struct allreduce_pr_req
{
    const void*  sendbuf;
    void*        recvbuf;
    void*        inbuf;
    int          count;
    MPI_Request  req[2];
    MPI_Datatype datatype;
    MPI_Op       op;
    MPI_Comm     comm;
    int          tag;
    int          phase;
    req_state    state;
} allreduce_pr_req;

typedef struct allreduce_pr_buf
{
    int   size;
    void* buf;
    int   in_use;
} allreduce_pr_buf;

void* allreduce_pr_get_buf(size_t);
void  allreduce_pr_release_buf(void*);
void  allreduce_pr_make_progress(int, int);
void  allreduce_pr_initialize();
int   allreduce_pr_is_done(int);
int   allreduce_pr_start(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*);
void  allreduce_pr_test(MPI_Request*, int*);

#endif /* ALLREDUCE_PR_H */
