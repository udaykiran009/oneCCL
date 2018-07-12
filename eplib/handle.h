#ifndef _HANDLE_H_
#define _HANDLE_H_

#include "client.h"
#include "cqueue.h"

/* Communicator values */
#define COMM_REG 0
#define COMM_EP  1

struct __handle_t
{
    int in_use;
    int type;                  /* Type of handle: COMM_REG or COMM_EP */
    MPI_Comm comm;             /* Current communicator handle */
    MPI_Comm parent_comm;      /* Parent Communicator */
    int local_epid;            /* Local id of communicator handle */
    int num_endpoints;         /* Number of local endpoints */
    int global_epid;           /* Global id of communicator handle */
    int tot_endpoints;         /* Total number of endpoints */
    cqueue_t* cqueue;          /* Pointer to command queue */
    MPI_Comm* worldcomm;       /* Server world communicator */
    MPI_Comm* peercomm;        /* Server peer endpoint communicator */
} __attribute__ ((aligned (CACHELINE_SIZE)));

typedef struct __handle_t handle_t;

void handle_init();
long handle_register(int, MPI_Comm, MPI_Comm, int, int, int, int, MPI_Comm* , MPI_Comm*);
int handle_get_type(MPI_Comm);
int handle_get_rank(MPI_Comm);
int handle_get_size(MPI_Comm);
cqueue_t* handle_get_cqueue(MPI_Comm);
MPI_Comm handle_get_server_comm(MPI_Comm, int);
MPI_Comm handle_get_server_worldcomm(MPI_Comm, int);
MPI_Comm handle_get_server_peercomm(MPI_Comm, int);
void handle_release(MPI_Comm);
void handle_finalize();

#endif /* _HANDLE_H_ */
