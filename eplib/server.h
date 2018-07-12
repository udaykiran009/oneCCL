#ifndef _SERVER_H_
#define _SERVER_H_

void server_create();
int server_memory_register(void*, size_t, MPI_Comm);
void server_destroy();

#endif /* _SERVER_H_ */
