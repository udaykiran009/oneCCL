__kernel void test_main(global int* this_send_buf,
                        global int* this_recv_buf,
                        global int* ipc_send_buf,
                        global int* ipc_recv_buf) {
    int thread_id = get_global_id(0);
    int data_offset = thread_id * sizeof(int16);
    printf("kernel wi: %d - offset: %d\n", thread_id, data_offset);

    int16 send_vector = vload16(data_offset, this_send_buf);
    int16 ipc_send_vector = vload16(data_offset, ipc_send_buf);
    int16 reduce_vector = send_vector + ipc_send_vector;

    printf("reduced: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
           reduce_vector[0],
           reduce_vector[1],
           reduce_vector[2],
           reduce_vector[3],
           reduce_vector[4],
           reduce_vector[5],
           reduce_vector[6],
           reduce_vector[7],
           reduce_vector[8],
           reduce_vector[9],
           reduce_vector[10],
           reduce_vector[11],
           reduce_vector[12],
           reduce_vector[13],
           reduce_vector[14],
           reduce_vector[15]);

    vstore16(reduce_vector, data_offset, ipc_recv_buf);
    vstore16(reduce_vector, data_offset, this_recv_buf);
}
