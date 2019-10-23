oneCCL Collective Communication Primitives
==========================================

oneAPI Collective Communication Library introduces the following list of communication primitives. 
These operations are collective, meaning that all participants of oneCCL communicator should make a call.

Allgatherv
**********

Collective communication operation that collects data from all processes within oneCCL communicator. 
Each participant gets the same result data. Different participants can contribute segments of different sizes.

**C version of oneCCL API:**

::

    ccl_status_t ccl_allgatherv(
        const void* send_buf,
        size_t send_count,
        void* recv_buf,
        const size_t* recv_counts,
        ccl_datatype_t dtype,
        const ccl_coll_attr_t* attr,
        ccl_comm_t comm,
        ccl_stream_t stream,
        ccl_request_t* req);

**C++ version of oneCCL API:**

::

    template<class buffer_type>
    coll_request_t communicator::allgatherv(const buffer_type* send_buf, 
                                size_t send_count,
                                buffer_type* recv_buf,
                                const size_t* recv_counts,
                                const ccl::coll_attr* attr,
                                const ccl::stream_t& stream);

send_buf
    the buffer with count elements of buffer_type that stores local data to be sent
recv_buf [out]
    the buffer to store received data, must have the same dimension as send_buf
count
    number of elements of type buffer_type to be sent by participant of oneCCL communicator
recv_counts
    number of elements of type buffer_type to be received from each participant of oneCCL communicator
dtype
    datatype of the elements (for C++ API gets inferred from the buffer type)
attr
    optional attributes that customize operation
comm
    oneCCL communicator for the operation
stream
    oneCCL stream associated with the operation
req
    object that can be used to track the progress of the operation (returned value for C++ API)



Allreduce
*********

Global reduction operations such as sum, max, min, or user-defined functions, where the result is returned to all members of oneCCL communicator.

**C version of oneCCL API:**

::

    ccl_status_t ccl_allreduce(
        const void* send_buf,
        void* recv_buf,
        size_t count,
        ccl_datatype_t dtype,
        ccl_reduction_t reduction,
        const ccl_coll_attr_t* attr,
        ccl_comm_t comm,
        ccl_stream_t stream,
        ccl_request_t* req);

**C++ version of oneCCL API:**

::

    template<class buffer_type>
    coll_request_t communicator::allreduce(const buffer_type* send_buf,
                            buffer_type* recv_buf,
                            size_t count,
                            ccl::reduction reduction,
                            const ccl::coll_attr* attr,
                            const ccl::stream_t& stream);

send_buf
    the buffer with count elements of buffer_type that stores local data to be reduced
recv_buf [out]
    the buffer to store reduced result, must have the same dimension as send_buf.
count
    number of elements of type buffer_type in send_buf
dtype
    datatype of the elements (for C++ API gets inferred from the buffer type)
reduction
    type of reduction operation to be applied
attr
    optional attributes that customize operation
comm
    oneCCL communicator for the operation
stream
    oneCCL stream associated with the operation
req
    object that can be used to track the progress of the operation (returned value for C++ API)

Reduce
******

Global reduction operations such as sum, max, min, or user-defined functions, where the result is returned to single member of oneCCL communicator (root).


**C version of oneCCL API:**

::

    ccl_status_t ccl_reduce(
        const void* send_buf,
        void* recv_buf,
        size_t count,
        ccl_datatype_t dtype,
        ccl_reduction_t reduction,
        size_t root,
        const ccl_coll_attr_t* attr,
        ccl_comm_t comm,
        ccl_stream_t stream,
        ccl_request_t* req);

**C++ version of oneCCL API:**

::

    template<class buffer_type>
    coll_request_t communicator::reduce(const buffer_type* send_buf,
                            buffer_type* recv_buf,
                            size_t count,
                            ccl::reduction reduction,
                            size_t root,
                            const ccl::coll_attr* attr,
                            const ccl::stream_t& stream);

send_buf
    the buffer with count elements of buffer_type that stores local data to be reduced
recv_buf [out]
    the buffer to store reduced result, must have the same dimension as send_buf.
count
    number of elements of type buffer_type in send_buf
dtype
    datatype of the elements (for C++ API gets inferred from the buffer type)
reduction
    type of reduction operation to be applied
root
    the rank of the process that gets result of reduction
attr
    optional attributes that customize operation
comm
    oneCCL communicator for the operation
stream
    oneCCL stream associated with the operation
req
    object that can be used to track the progress of the operation (returned value for C++ API)

The following reduction operations are supported for Allreduce and Reduce primitives:

ccl_reduction_sum
    Elementwise summation
ccl_reduction_prod
    Elementwise multiplication
ccl_reduction_min
    Elementwise min
ccl_reduction_max
    Elementwise max
ccl_reduction_custom:
    Class of user-defined operations

Alltoall
********

oneCCL Alltoall is an extension of oneCCL Allgather to the case where each process
sends distinct data to each of the receivers. The j-th block sent from process i is received
by process j and is placed in the i-th block of recvbuf. Amount of data sent must be equal to 
the amount of data received, pairwise between every pair of processes.

**C version of oneCCL API:**

::

    ccl_status_t ccl_alltoall(
                    const void* send_buf,
                    void* recv_buf,
                    size_t count,
                    ccl_datatype_t dtype,
                    const ccl_coll_attr_t* attr,
                    ccl_comm_t comm,
                    ccl_stream_t stream,
                    ccl_request_t* req);

**C++ version of oneCCL API:**

::

    template<class buffer_type>
    coll_request_t communicator::alltoall(const buffer_type* send_buf,
                                        buffer_type* recv_buf,
                                        size_t count,
                                        const ccl::coll_attr* attr,
                                        const ccl::stream_t& stream);


send_buf
    the buffer with count elements of buffer_type that stores local data to be sent
recv_buf [out]
    the buffer to store received data, must have the same dimension as send_buf.
count
    number of elements of type buffer_type to be sent to/received from each participant of oneCCL communicator
dtype
    datatype of the elements (for C++ API gets inferred from the buffer type)
attr
    optional attributes that customize operation
comm
    oneCCL communicator for the operation
stream
    oneCCL stream associated with the operation
req
    object that can be used to track the progress of the operation (returned value for C++ API)

Barrier
*******

Blocking barrier synchronization across all members of oneCCL communicator.

**C version of oneCCL API:**

::

    ccl_status_t ccl_barrier(ccl_comm_t comm,
                            ccl_stream_t stream);

**C++ version of oneCCL API:**

::

    void communicator::barrier(const ccl::stream_t& stream);

comm
    oneCCL communicator for the operation
stream
    oneCCL stream associated with the operation

Broadcast
*********

Collective communication operation that broadcasts data from one participant of communicator (denoted as root) to all participants.

**C version of oneCCL API:**

::

    ccl_status_t ccl_bcast(
        void* buf,
        size_t count,
        ccl_datatype_t dtype,
        size_t root,
        const ccl_coll_attr_t* attr,
        ccl_comm_t comm,
        ccl_stream_t stream,
        ccl_request_t* req);

**C++ version of oneCCL API:**

::

    template<class buffer_type>
    col_request_t communicator::bcast(buffer_type* buf, size_t count,
                            size_t root,
                            const ccl::coll_attr* attr,
                            const ccl::stream_t& stream);

buf
    serves as send buffer for root and as receive buffer for other participants
count
    number of elements of type buffer_type in send_buf
dtype
    datatype of the elements (for C++ API gets inferred from the buffer type)
root
    the rank of the process that broadcasts the data
attr
    optional attributes that customize the operation
comm
    oneCCL communicator for the operation
stream
    oneCCL stream associated with the operation
req
    object that can be used to track the progress of the operation (returned value for C++ API)
