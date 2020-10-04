#include "sycl_base.hpp"

using native_dtype = int32_t;
using namespace std;
using namespace sycl;

struct custom_data_type {
    native_dtype arr[sizeof(native_dtype)];
} __attribute__((packed));

int main(int argc, char *argv[]) {

    const size_t count = 10 * 1024 * 1024;

    int i = 0;
    int size = 0;
    int rank = 0;

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    queue q;
    if (!create_sycl_queue(argc, argv, q)) {
        MPI_Finalize();
        return -1;
    }

    buf_allocator<int> allocator(q);

    auto usm_alloc_type = usm::alloc::shared;
    if (argc > 2) {
        usm_alloc_type = usm_alloc_type_from_string(argv[2]);
    }

    constexpr size_t send_count = count * sizeof(custom_data_type) / sizeof(native_dtype);

    auto send_buf = allocator.allocate(send_count, usm_alloc_type);
    auto recv_buf = allocator.allocate(send_count * size, usm_alloc_type);

    buffer<int> expected_buf(send_count * size);
    buffer<int> check_buf(send_count * size);

    /* create kvs */
    ccl::shared_ptr_class<ccl::kvs> kvs;
    ccl::kvs::address_type main_addr;
    if (rank == 0) {
        kvs = ccl::create_main_kvs();
        main_addr = kvs->get_address();
        MPI_Bcast((void *)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Bcast((void *)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs = ccl::create_kvs(main_addr);
    }

    /* create communicator */
    auto dev = ccl::create_device(q.get_device());
    auto ctx = ccl::create_context(q.get_context());
    auto comm = ccl::create_communicator(size, rank, dev, ctx, kvs);

    /* create stream */
    auto stream = ccl::create_stream(q);

    vector<size_t> recv_counts(size, send_count);

    /* open send_buf and modify it on the device side */
    auto e = q.submit([&](auto &h) {
        accessor expected_buf_acc(expected_buf, h, write_only);
        h.parallel_for(send_count, [=](auto id) {
                static_cast<native_dtype *>(send_buf)[id] = rank + 1;
                for (int i = 0; i < size; i++) {
                    static_cast<native_dtype *>(recv_buf)[id] = -1;
                    expected_buf_acc[i * send_count + id] = i + 1;
                }
            });
    });

    /* create dependency vector */
    vector<ccl::event> events;
    //events.push_back(ccl::create_event(e));

    if (!handle_exception(q))
        return -1;

    /* invoke allgatherv */
    auto attr = ccl::create_operation_attr<ccl::allgatherv_attr>();
    ccl::allgatherv(send_buf,
                    send_count,
                    recv_buf,
                    recv_counts,
                    ccl::datatype::int32,
                    comm,
                    stream,
                    attr,
                    events)
        .wait();

    /* open recv_buf and check its correctness on the device side */
    q.submit([&](auto &h) {
        accessor expected_buf_acc(expected_buf, h, read_only);
        accessor check_buf_acc(check_buf, h, write_only);
        h.parallel_for(size * send_count, [=](auto id) {
                if (static_cast<native_dtype *>(recv_buf)[id] != expected_buf_acc[id]) {
                    check_buf_acc[id] = -1;
                }
            });
    });

    if (!handle_exception(q))
        return -1;

    /* print out the result of the test on the host side */
    if (rank == COLL_ROOT) {
        host_accessor check_buf_acc(check_buf, read_only);
        for (i = 0; i < size * send_count; i++) {
            if (check_buf_acc[i] == -1) {
                cout << "FAILED";
                break;
            }
        }
        if (i == size * send_count) {
            cout << "PASSED\n";
        }
    }

    MPI_Finalize();

    return 0;
}
