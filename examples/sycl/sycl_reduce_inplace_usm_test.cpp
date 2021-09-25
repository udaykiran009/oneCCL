#include "sycl_base.hpp"

using namespace std;
using namespace sycl;

int main(int argc, char* argv[]) {
    const size_t count = 10 * 1024 * 1024;
    int root_rank = 1;

    int size = 0;
    int rank = 0;

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    atexit(mpi_finalize);

    queue q;
    if (!create_sycl_queue(argc, argv, rank, q)) {
        return -1;
    }

    buf_allocator<int> allocator(q);

    auto usm_alloc_type = usm::alloc::shared;
    if (argc > 2) {
        usm_alloc_type = usm_alloc_type_from_string(argv[2]);
    }
    if (argc > 3) {
        root_rank = atoi(argv[3]);
    }
    if (rank == root_rank) {
        printf("root rank: %d\n", root_rank);
    }

    if (!check_sycl_usm(q, usm_alloc_type)) {
        return -1;
    }

    /* create kvs */
    ccl::shared_ptr_class<ccl::kvs> kvs;
    ccl::kvs::address_type main_addr;
    if (rank == 0) {
        kvs = ccl::create_main_kvs();
        main_addr = kvs->get_address();
        MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs = ccl::create_kvs(main_addr);
    }

    /* create communicator */
    auto dev = ccl::create_device(q.get_device());
    auto ctx = ccl::create_context(q.get_context());
    auto comm = ccl::create_communicator(size, rank, dev, ctx, kvs);

    /* create stream */
    auto stream = ccl::create_stream(q);

    /* create buffers */
    auto buf = allocator.allocate(count, usm_alloc_type);

    /* open buffers and modify them on the device side */
    auto e = q.submit([&](auto& h) {
        h.parallel_for(count, [=](auto id) {
            buf[id] = rank + id + 1;
        });
    });

    int check_sum = 0;
    for (int i = 1; i <= size; ++i) {
        check_sum += i;
    }

    /* do not wait completion of kernel and provide it as dependency for operation */
    vector<ccl::event> deps;
    deps.push_back(ccl::create_event(e));

    /* invoke allreduce */
    auto attr = ccl::create_operation_attr<ccl::reduce_attr>();
    ccl::reduce(buf, buf, count, ccl::reduction::sum, root_rank, comm, stream, attr, deps).wait();

    /* open buf and check its correctness on the device side */
    buffer<int> check_buf(count);

    q.submit([&](auto& h) {
        accessor check_buf_acc(check_buf, h, write_only);
        h.parallel_for(count, [=](auto id) {
            int expected = (rank == root_rank) ? (check_sum + size * id) : (rank + id + 1);
            if (buf[id] != expected) {
                check_buf_acc[id] = -1;
            }
            else {
                check_buf_acc[id] = 0;
            }
        });
    });

    if (!handle_exception(q))
        return -1;

    /* print out the result of the test on the host side */
    {
        host_accessor check_buf_acc(check_buf, read_only);
        size_t i;
        for (i = 0; i < count; i++) {
            if (check_buf_acc[i] == -1) {
                cout << "FAILED\n";
                break;
            }
        }
        if (i == count) {
            cout << "PASSED\n";
        }
    }

    return 0;
}
