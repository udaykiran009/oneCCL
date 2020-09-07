#pragma once

//API headers with declaration of new API object
#define private public
#define protected public
#include "ccl_types.hpp"
#include "ccl_aliases.hpp"

#include "ccl_type_traits.hpp"
#include "ccl_types_policy.hpp"

#include "ccl_comm_split_attr_ids.hpp"
#include "ccl_comm_split_attr_ids_traits.hpp"
#include "ccl_comm_split_attr.hpp"
#include "comm_split_attr_creation_impl.hpp"

#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"
#include "ccl_coll_attr.hpp"

#include "coll/coll_attributes.hpp"
#include "coll_attr_creation_impl.hpp"

#include "ccl_event_attr_ids.hpp"
#include "ccl_event_attr_ids_traits.hpp"
#include "ccl_event.hpp"

#include "ccl_stream_attr_ids.hpp"
#include "ccl_stream_attr_ids_traits.hpp"
#include "ccl_stream.hpp"

#include "communicator_impl.hpp"

// #include "../stream/environment.hpp"
// #include "native_device_api/export_api.hpp"

#include "../stubs/kvs.hpp"
#include "common/global/global.hpp"

namespace host_communicator_suite
{

TEST(host_communicator_api, host_comm_creation)
{
    ccl::global_data::get().init(); // TODO must be a part of the environment class

    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(2, 1, stub_storage);
    ASSERT_EQ(comm.size(), 2);
    ASSERT_EQ(comm.rank(), 1);
}

TEST(host_communicator_api, move_host_comm)
{
    /* move constructor test */
    std::shared_ptr<stub_kvs> stub_storage;
    auto orig_comm = ccl::communicator::create_communicator(2, 1, stub_storage);

    auto orig_inner_impl_ptr = orig_comm.get_impl().get();
    auto moved_comm = (std::move(orig_comm));
    auto moved_inner_impl_ptr = moved_comm.get_impl().get();

    ASSERT_EQ(orig_inner_impl_ptr, moved_inner_impl_ptr);
    ASSERT_TRUE(moved_comm.get_impl());
    ASSERT_TRUE(!orig_comm.get_impl());
    ASSERT_EQ(moved_comm.size(), 2);
    ASSERT_EQ(moved_comm.rank(), 1);

    /* move assignment test*/
    auto orig_comm2 = ccl::communicator::create_communicator(2, 1, stub_storage);
    auto moved_comm2 = ccl::communicator::create_communicator(4, 3, stub_storage);;
    moved_comm2 = std::move(orig_comm2);

    ASSERT_TRUE(moved_comm2.get_impl());
    ASSERT_TRUE(!orig_comm2.get_impl());
    ASSERT_EQ(moved_comm2.rank(), 1);
    ASSERT_EQ(moved_comm2.size(), 2);
}

TEST(host_communicator_api, host_comm_split)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(2, 1, stub_storage);
    auto attr = ccl::create_comm_split_attr(
                    ccl::attr_arg<ccl::ccl_comm_split_attributes::color>(1)
                );
    auto new_comm = comm.split(attr);
    ASSERT_EQ(new_comm.size(), 1);
}

TEST(host_communicator_api, host_comm_allgatherv_void)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    void* send_buf = nullptr;
    void* recv_buf = nullptr;
    size_t send_count = 0;
    const ccl::vector_class<size_t> recv_counts;
    ccl_datatype_t dtype = ccl_dtype_int;
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr_t>();

    auto req = comm.allgatherv(
                    send_buf, send_count, recv_buf, recv_counts, dtype, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_allgatherv_int)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    int* send_buf = nullptr;
    int* recv_buf = nullptr;
    size_t send_count = 0;
    const ccl::vector_class<size_t> recv_counts;
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr_t>();

    auto req = comm.allgatherv(
                    send_buf, send_count, recv_buf, recv_counts, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_allgatherv_void_recv_bufs)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    void* send_buf = nullptr;
    ccl::vector_class<void*> recv_bufs;
    size_t send_count = 0;
    const ccl::vector_class<size_t> recv_counts;
    ccl_datatype_t dtype = ccl_dtype_int;
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr_t>();

    auto req = comm.allgatherv(
                    send_buf, send_count, recv_bufs, recv_counts, dtype, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_allgatherv_int_recv_bufs)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    int* send_buf = nullptr;
    ccl::vector_class<int*> recv_bufs;
    size_t send_count = 0;
    const ccl::vector_class<size_t> recv_counts;
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr_t>();

    auto req = comm.allgatherv(
                    send_buf, send_count, recv_bufs, recv_counts, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_allreduce_void)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    void* send_buf = nullptr;
    void* recv_buf = nullptr;
    size_t count = 0;
    ccl_datatype_t dtype = ccl_dtype_int;
    ccl_reduction_t reduction = ccl_reduction_sum;
    auto attr = ccl::create_coll_attr<ccl::allreduce_attr_t>();

    auto req = comm.allreduce(
                    send_buf, recv_buf, count, dtype, reduction, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_allreduce_int)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    int* send_buf = nullptr;
    int* recv_buf = nullptr;
    size_t count = 0;
    ccl_reduction_t reduction = ccl_reduction_sum;
    auto attr = ccl::create_coll_attr<ccl::allreduce_attr_t>();

    auto req = comm.allreduce(
                    send_buf, recv_buf, count, reduction, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_alltoall_void)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    void* send_buf = nullptr;
    void* recv_buf = nullptr;
    size_t count = 0;
    ccl_datatype_t dtype = ccl_dtype_int;
    auto attr = ccl::create_coll_attr<ccl::alltoall_attr_t>();

    auto req = comm.alltoall(
                    send_buf, recv_buf, count, dtype, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_alltoall_void_vector_bufs)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    ccl::vector_class<void*> send_buf;
    ccl::vector_class<void*> recv_buf;
    size_t count = 0;
    ccl_datatype_t dtype = ccl_dtype_int;
    auto attr = ccl::create_coll_attr<ccl::alltoall_attr_t>();

    auto req = comm.alltoall(
                    send_buf, recv_buf, count, dtype, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_alltoall_int)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    int* send_buf = nullptr;
    int* recv_buf = nullptr;
    size_t count = 0;
    auto attr = ccl::create_coll_attr<ccl::alltoall_attr_t>();

    auto req = comm.alltoall(
                    send_buf, recv_buf, count, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_alltoall_int_vector_bufs)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    ccl::vector_class<int*> send_buf;
    ccl::vector_class<int*> recv_buf;
    size_t count = 0;
    auto attr = ccl::create_coll_attr<ccl::alltoall_attr_t>();

    auto req = comm.alltoall(
                    send_buf, recv_buf, count, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_alltoallv_void)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    void* send_buf = nullptr;
    void* recv_buf = nullptr;
    ccl::vector_class<size_t> send_counts;
    ccl::vector_class<size_t> recv_counts;
    ccl_datatype_t dtype = ccl_dtype_int;
    auto attr = ccl::create_coll_attr<ccl::alltoallv_attr_t>();

    auto req = comm.alltoallv(
                    send_buf, send_counts, recv_buf, recv_counts, dtype, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_alltoallv_void_recv_bufs)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    ccl::vector_class<void*> send_bufs;
    ccl::vector_class<void*> recv_bufs;
    ccl::vector_class<size_t> send_counts;
    ccl::vector_class<size_t> recv_counts;
    ccl_datatype_t dtype = ccl_dtype_int;
    auto attr = ccl::create_coll_attr<ccl::alltoallv_attr_t>();

    auto req = comm.alltoallv(
                    send_bufs, send_counts, recv_bufs, recv_counts, dtype, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_alltoallv_int)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    int* send_buf = nullptr;
    int* recv_buf = nullptr;
    ccl::vector_class<size_t> send_counts;
    ccl::vector_class<size_t> recv_counts;
    auto attr = ccl::create_coll_attr<ccl::alltoallv_attr_t>();

    auto req = comm.alltoallv(
                    send_buf, send_counts, recv_buf, recv_counts, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_alltoallv_int_recv_bufs)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    ccl::vector_class<int*> send_bufs;
    ccl::vector_class<int*> recv_bufs;
    ccl::vector_class<size_t> send_counts;
    ccl::vector_class<size_t> recv_counts;
    auto attr = ccl::create_coll_attr<ccl::alltoallv_attr_t>();

    auto req = comm.alltoallv(
                    send_bufs, send_counts, recv_bufs, recv_counts, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_barrier)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    auto attr = ccl::create_coll_attr<ccl::barrier_attr_t>();

    auto req = comm.barrier(attr);
    req->wait();
}

TEST(host_communicator_api, host_comm_bcast_void)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    void* buf = nullptr;
    size_t count = 0;
    ccl_datatype_t dtype = ccl_dtype_int;
    size_t root = 0;
    auto attr = ccl::create_coll_attr<ccl::bcast_attr_t>();

    auto req = comm.bcast(
                    buf, count, dtype, root, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_bcast_int)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    int* buf = nullptr;
    size_t count = 0;
    size_t root = 0;
    auto attr = ccl::create_coll_attr<ccl::bcast_attr_t>();

    auto req = comm.bcast(
                    buf, count, root, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_reduce_void)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    void* send_buf = nullptr;
    void* recv_buf = nullptr;
    size_t count = 0;
    ccl_datatype_t dtype = ccl_dtype_int;
    ccl_reduction_t reduction = ccl_reduction_sum;
    size_t root = 0;
    auto attr = ccl::create_coll_attr<ccl::reduce_attr_t>();

    auto req = comm.reduce(
                    send_buf, recv_buf, count, dtype, reduction, root, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_reduce_int)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    int* send_buf = nullptr;
    int* recv_buf = nullptr;
    size_t count = 0;
    ccl_reduction_t reduction = ccl_reduction_sum;
    size_t root = 0;
    auto attr = ccl::create_coll_attr<ccl::reduce_attr_t>();

    auto req = comm.reduce(
                    send_buf, recv_buf, count, reduction, root, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_reduce_scatter_void)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    void* send_buf = nullptr;
    void* recv_buf = nullptr;
    size_t recv_count = 0;
    ccl_datatype_t dtype = ccl_dtype_int;
    ccl_reduction_t reduction = ccl_reduction_sum;
    auto attr = ccl::create_coll_attr<ccl::reduce_scatter_attr_t>();

    auto req = comm.reduce_scatter(
                    send_buf, recv_buf, recv_count, dtype, reduction, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_reduce_scatter_int)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    int* send_buf = nullptr;
    int* recv_buf = nullptr;
    size_t recv_count = 0;
    ccl_reduction_t reduction = ccl_reduction_sum;
    auto attr = ccl::create_coll_attr<ccl::reduce_scatter_attr_t>();

    auto req = comm.reduce_scatter(
                    send_buf, recv_buf, recv_count, reduction, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_sparse_allreduce_void)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    void* send_ind_buf = nullptr;
    size_t send_ind_count = 0;
    void* send_val_buf = nullptr;
    size_t send_val_count = 0;
    void* recv_ind_buf = nullptr;
    size_t recv_ind_count = 0;
    void* recv_val_buf = nullptr;
    size_t recv_val_count = 0;
    ccl_datatype_t ind_dtype = ccl_dtype_int;
    ccl_datatype_t val_dtype = ccl_dtype_int;
    ccl_reduction_t reduction = ccl_reduction_sum;
    auto attr = ccl::create_coll_attr<ccl::sparse_allreduce_attr_t>();

    auto req = comm.sparse_allreduce(
                    send_ind_buf, send_ind_count, send_val_buf, send_val_count,
                    recv_ind_buf, recv_ind_count, recv_val_buf, recv_val_count,
                    ind_dtype, val_dtype, reduction, attr
                );
    req->wait();
}

TEST(host_communicator_api, host_comm_sparse_allreduce_int)
{
    std::shared_ptr<stub_kvs> stub_storage;
    auto comm = ccl::communicator::create_communicator(1, 0, stub_storage);

    int* send_ind_buf = nullptr;
    size_t send_ind_count = 0;
    int* send_val_buf = nullptr;
    size_t send_val_count = 0;
    int* recv_ind_buf = nullptr;
    size_t recv_ind_count = 0;
    int* recv_val_buf = nullptr;
    size_t recv_val_count = 0;
    ccl_reduction_t reduction = ccl_reduction_sum;
    auto attr = ccl::create_coll_attr<ccl::sparse_allreduce_attr_t>();

    auto req = comm.sparse_allreduce(
                    send_ind_buf, send_ind_count, send_val_buf, send_val_count,
                    recv_ind_buf, recv_ind_count, recv_val_buf, recv_val_count,
                    reduction, attr
                );
    req->wait();
}

} // namespace host_communicator_suite
