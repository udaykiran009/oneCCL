#pragma once

#include "ccl_types.hpp"

#include <memory>

class ccl_comm;
class ccl_stream;

namespace ccl
{

/**
 * ccl environemt. The user must guarantee that the only instance of this class exists
 * during application life time.
 */
class environment
{
public:
    environment();

    ~environment();

private:
    static bool is_initialized;
};

/**
 * A request object that allows the user to track collective operation progress
 */
class request
{
public:
    /**
     * Blocking wait for collective operation completion
     */
    virtual void wait() = 0;

    /**
     * Non-blocking check for collective operation completion
     * @retval true if the operations has been completed
     * @retval false if the operations has not been completed
     */
    virtual bool test() = 0;

    virtual ~request() = default;
};

/**
 * A stream object is an abstraction over CPU/GPU streams
 */
class stream
{
public:
    stream();

    stream(ccl::stream_type type, void* native_stream);

private:
    std::shared_ptr<ccl_stream> stream_impl;

    friend class communicator;
};

/**
 * A communicator that permits collective operations
 */
class communicator
{
public:
    /**
     * Creates ccl communicator as a copy of global communicator
     */
    communicator();

    /**
     * Creates a new communicator according to @c attr parameters
     * @param attr
     */
    explicit communicator(const ccl::comm_attr* attr);

    /**
     * Retrieves the rank of the current process in a communicator
     * @return rank of the current process
     */
    size_t rank();

    /**
     * Retrieves the number of processes in a communicator
     * @return number of the processes
     */
    size_t size();

    /**
     * Gathers @c buf on all process in the communicator and stores result in @c recv_buf
     * on each process
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_buf [out] the buffer to store gathered result on the @c each process, must have the same dimension
     * as @c buf. Used by the @c root process only, ignored by other processes
     * @param recv_counts array with number of elements received by each process
     * @param dtype data type of elements in the buffer @c buf and @c recv_buf
     * @param attr optional attributes that customize operation
     * @return @ref ccl::request object that can be used to track the progress of the operation
     */
    std::shared_ptr<ccl::request> allgatherv(const void* send_buf,
                                             size_t send_count,
                                             void* recv_buf,
                                             const size_t* recv_counts,
                                             ccl::data_type dtype,
                                             const ccl::coll_attr* attr = nullptr,
                                             const ccl::stream* stream = nullptr);

    /**
     * Reduces @c buf on all process in the communicator and stores result in @c recv_buf
     * on each process
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] - the buffer to store reduced result , must have the same dimension
     * as @c buf.
     * @param count number of elements of type @c dtype in @c buf
     * @param dtype data type of elements in the buffer @c buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param attr optional attributes that customize operation
     * @return @ref ccl::request object that can be used to track the progress of the operation
     */
    std::shared_ptr<ccl::request> allreduce(const void* send_buf,
                                            void* recv_buf,
                                            size_t count,
                                            ccl::data_type dtype,
                                            ccl::reduction reduction,
                                            const ccl::coll_attr* attr = nullptr,
                                            const ccl::stream* stream = nullptr);

    /**
     * Collective operation that blocks each process until every process have reached it
     */
    void barrier(const ccl::stream* stream = nullptr);

    /**
     * Broadcasts @c buf from the @c root process to other processes in a communicator
     * @param buf [in,out] the buffer with @c count elements of @c dtype to be transmitted
     * if the rank of the communicator is equal to @c root or to be received by other ranks
     * @param count number of elements of type @c dtype in @c buf
     * @param dtype data type of elements in the buffer @c buf
     * @param root the rank of the process that will transmit @c buf
     * @param attr optional attributes that customize operation
     * @return @ref ccl::request object that can be used to track the progress of the operation
     */
    std::shared_ptr<ccl::request> bcast(void* buf,
                                        size_t count,
                                        ccl::data_type dtype,
                                        size_t root,
                                        const ccl::coll_attr* attr = nullptr,
                                        const ccl::stream* stream = nullptr);

    /**
     * Reduces @c buf on all process in the communicator and stores result in @c recv_buf
     * on the @c root process
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result on the @c root process, must have the same dimension
     * as @c buf. Used by the @c root process only, ignored by other processes
     * @param count number of elements of type @c dtype in @c buf
     * @param dtype data type of elements in the buffer @c buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param root the rank of the process that will held result of reduction
     * @param attr optional attributes that customize operation
     * @return @ref ccl::request object that can be used to track the progress of the operation
     */
    std::shared_ptr<ccl::request> reduce(const void* send_buf,
                                         void* recv_buf,
                                         size_t count,
                                         ccl::data_type dtype,
                                         ccl::reduction reduction,
                                         size_t root,
                                         const ccl::coll_attr* attr = nullptr,
                                         const ccl::stream* stream = nullptr);

    /**
     * Reduces sparse @c buf on all process in the communicator and stores result in @c recv_buf
     * on each process
     * @param send_ind_buf the buffer of indices with @c send_ind_count elements of @c index_dtype
     * @param send_int_count number of elements of type @c index_type @c send_ind_buf
     * @param send_val_buf the buffer of values with @c send_val_count elements of @c value_dtype
     * @param send_val_count number of elements of type @c value_type @c send_val_buf
     * @param recv_ind_buf [out] the buffer to store reduced indices, must have the same dimension as @c send_ind_buf
     * @param recv_ind_count [out] the amount of reduced indices
     * @param recv_val_buf [out] the buffer to store reduced values, must have the same dimension as @c send_val_buf
     * @param recv_val_count [out] the amount of reduced values
     * @param index_dtype index type of elements in the buffer @c send_ind_buf and @c recv_ind_buf
     * @param value_dtype data type of elements in the buffer @c send_val_buf and @c recv_val_buf
     * @param reduction type of reduction operation to be applied
     * @param attr optional attributes that customize operation
     * @return @ref ccl::request object that can be used to track the progress of the operation
     */
    std::shared_ptr<ccl::request> sparse_allreduce(const void* send_ind_buf, size_t send_ind_count,
                                                   const void* send_val_buf, size_t send_val_count,
                                                   void** recv_ind_buf, size_t* recv_ind_count,
                                                   void** recv_val_buf, size_t* recv_val_count,
                                                   ccl::data_type index_dtype,
                                                   ccl::data_type value_dtype,
                                                   ccl::reduction reduction,
                                                   const ccl::coll_attr* attr = nullptr,
                                                   const ccl::stream* stream = nullptr);

private:
    std::shared_ptr<ccl_comm> comm_impl;
};

}
