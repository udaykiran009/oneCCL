#ifndef _WINDOW_H_
#define _WINDOW_H_

#include "client.h"

struct __window_t
{
    int in_use;
    MPI_Win win;
    void* base;
    client_t* client;        /* Pointer to client */
    int free_at_release;
    int pad[9];
};

typedef struct __window_t window_t;

void window_init();
void window_register(MPI_Win*, void*, client_t*, int);
client_t* window_get_client(MPI_Win);
void* window_get_baseptr(MPI_Win);
MPI_Win window_get_server_win(MPI_Win);
void window_release(int);
void window_finalize();

#endif /* _WINDOW_H_ */
