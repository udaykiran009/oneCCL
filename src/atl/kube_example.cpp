#include <dlfcn.h>
#include <signal.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <atl.h>

struct mlsl_executor
{
    atl_desc_t *atl_desc;
    atl_comm_t **atl_comms;
    // TODO_ATL: add corresponding ATL methods like MPI_Comm_rank/size
    size_t proc_idx;
    size_t proc_count;
    size_t worker_count;
};

typedef struct mlsl_executor mlsl_executor;

static int cont = 1;
static int to_up = 0;
static int print_first_iter = 2;
static mlsl_executor *e;
struct sigaction old_act,old_act2,old_act3;

#define MAX_KVS_NAME_LENGTH 128

void sig_up(int sig)
{
    printf("sig_up %d\n\n", sig);
    fprintf(stderr, "sig_up %d\n\n", sig);
    old_act2.sa_handler(sig);
    to_up = 1;
    print_first_iter = 2;
}

void fri(int sig)
{
    printf("fri %d\n\n", sig);
    fprintf(stderr, "fri %d\n\n", sig);
    if (sig == SIGTERM)
        old_act3.sa_handler(sig);
    printf("run\n\n");
    cont = 0;
    print_first_iter = 2;
}

framework_answers_t default_checker(size_t comm_size)
{
    onst size_t comm_size_to_start = 8;

    if (comm_size >= comm_size_to_start)
            return FA_USE;

    return FA_WAIT;
}

int main(int argc, char* argv[])
{
    struct sigaction act,act2;
    int status = 0;

    int send_to;
    int recv_from;
    atl_attr_t attr = { .comm_count = 1 };
        void *dlhandle;

    char send[5], recv[5];
    int  recv_i;
    atl_req_t* req = (atl_req_t*)malloc(2*sizeof(atl_req_t));
   // req.tag = 0;
    send[0] = 'a';
    send[1] = 'b';
    send[2] = 'c';
    send[3] = 'd';
    send[4] = '\0';
    e = (mlsl_executor *)malloc(sizeof(mlsl_executor));
    e->worker_count = 1;
printf("Init\n");
    atl_status_t atl_status = atl_init(NULL, NULL,  &e->proc_idx, &e->proc_count, &attr, &e->atl_comms, &e->atl_desc);

        memset(&act, 0, sizeof(act));
        act.sa_handler = &fri;
        act.sa_flags = 0;
        sigaction(SIGTERM, &act, &old_act3);

        sigaction(SIGINT, &act, &old_act);

        memset(&act2, 0, sizeof(act2));
        act2.sa_handler = &sig_up;
        act2.sa_flags = 0;
        sigaction(SIGUSR1, &act2, &old_act2);

    if (atl_status != atl_status_success)
           exit(1);

    atl_status = atl_set_framework_function(e->atl_desc, &default_checker);

    send_to = (e->proc_idx + 1) % e->proc_count;
    recv_from = e->proc_idx ? (e->proc_idx - 1) % e->proc_count : e->proc_count - 1 ;
    printf("Start\n");

    while (cont == 1)
    {
        usleep(10000);
        atl_status = atl_comm_send(e->atl_comms[0], (void*)(&(e->proc_idx)), 1, send_to,0, &req[0]);
        if (to_up == 1)
            goto up_step;
        if (cont == 0)
            goto fin_step;

        atl_comm_recv(e->atl_comms[0], (void*)(&recv_i), 1, recv_from, 0, &req[1]);

        if (to_up == 1)
            goto up_step;
        if (cont == 0)
            goto fin_step;
        if(print_first_iter > 0)
        {
            printf("send_recv pass\n\n");
            print_first_iter--;
        }
up_step:
        if(to_up == 1)
        {
printf("To up\n");
          atl_update(&e->proc_idx, &e->proc_count, e->atl_desc);
          send_to = (e->proc_idx + 1) % e->proc_count;
          recv_from = e->proc_idx ? (e->proc_idx - 1) % e->proc_count : e->proc_count - 1 ;
          to_up = 0;
printf("\n\nDONE\n\n");
        }
    }

fin_step:
    atl_finalize(e->atl_desc, e->atl_comms);
    free(e);
    free(req);
    return 0;
}
