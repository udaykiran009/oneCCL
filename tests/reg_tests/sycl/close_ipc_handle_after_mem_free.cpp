#include "sycl_base.hpp"

using namespace std;
using namespace sycl;

int main(int argc, char* argv[]) {
    const size_t count = 1024;

    int size = 0;
    int rank = 0;

    sycl::queue q;
    if (!q.get_device().is_gpu()) {
        cout << "test expects GPU device, please use SYCL_DEVICE_FILTER accordingly";
        return -1;
    }

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    atexit(mpi_finalize);

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
    auto send_buf = sycl::malloc_device<int>(count, q);
    auto recv_buf = sycl::malloc_device<int>(count, q);

    /* open buffers and modify them on the device side */
    auto e = q.submit([&](auto& h) {
        h.parallel_for(count, [=](auto id) {
            send_buf[id] = rank + id + 1;
            recv_buf[id] = -1;
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
    auto attr = ccl::create_operation_attr<ccl::allreduce_attr>();
    ccl::allreduce(send_buf, recv_buf, count, ccl::reduction::sum, comm, stream, attr, deps).wait();

    /* open recv_buf and check its correctness on the device side */
    buffer<int> check_buf(count);
    q.submit([&](auto& h) {
        accessor check_buf_acc(check_buf, h, write_only);
        h.parallel_for(count, [=](auto id) {
            if (recv_buf[id] != static_cast<int>(check_sum + size * id)) {
                check_buf_acc[id] = -1;
            }
            else {
                check_buf_acc[id] = 0;
            }
        });
    });

    if (!handle_exception(q)) {
        return -1;
    }

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

    /**
     * The point of the test is to free memory on the exporter process side
     * before the IPC handle of this memory is closed on the importer process side.
     * IPC handles will be closed when clearing the ze_cache i.e. at the end of the program.
     * Thus, we check the correctness of such a scenario.
     */
    if (rank == 0) {
        sycl::free(send_buf, q);
        sycl::free(recv_buf, q);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    return 0;
}
