#include <stdio.h>

#include "ccl.h"

#define COUNT 2

int main(int argc, char** argv) {
    int i = 0;
    size_t size = 0;
    size_t rank = 0;
    ccl_request_t request;
    ccl_stream_t stream;
    int sendbuf[COUNT];
    int* recvbuf;
	size_t *recv_counts;
	
    ccl_init();

    ccl_get_comm_rank(NULL, &rank);
    ccl_get_comm_size(NULL, &size);

    /* create CPU stream */
    ccl_stream_create(ccl_stream_cpu, NULL, &stream);

    /* initialize sendbuf */
    for (i = 0; i < COUNT; i++) {
        sendbuf[i] = rank;
    }
	for (i = 0; i < COUNT; i++) 
		printf("sendbuf[%d] = %d\n", i, sendbuf[i]);
    /* modify sendbuf */
    for (i = 0; i < COUNT; i++) {
        sendbuf[i] += 1;
    }
	for (i = 0; i < COUNT; i++) 
		printf("sendbuf+1[%d] = %d\n", i, sendbuf[i]);
	recvbuf = malloc(size * COUNT * sizeof(int));
    recv_counts = malloc(size * sizeof(size_t));

    for (i = 0; i < size; i++)
        recv_counts[i] = COUNT;
    /* invoke ccl_allreduce */
    ccl_allgatherv(sendbuf,
			      COUNT,
                  recvbuf,
                  recv_counts,
                  ccl_dtype_int,
                  NULL,
                  NULL,
                  stream,
                  &request);

    ccl_wait(request);
	for (i = 0; i < COUNT * size; i++) 
		printf("recvbuf[%d] = %d\n", i, recvbuf[i]);
    /* check correctness of recvbuf */
    for (i = 0; i < COUNT; i++) {
       if (recvbuf[i] != rank + 1) {
           recvbuf[i] = -1;
       }
    }

    /* print out the result of the test */
    if (rank == 0){
        for (i = 0; i < COUNT; i++) {
            if (recvbuf[i] == -1) {
                printf("FAILED\n");
                break;
            }
        }
        if (i == COUNT) {
            printf("PASSED\n");
        }
    }

    ccl_stream_free(stream);

    ccl_finalize();

    return 0;
}
