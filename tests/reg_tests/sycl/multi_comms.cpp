#include "sycl_base.hpp"

using namespace std;
using namespace sycl;

std::vector<int> comm_sizes{};
bool reorder_ranks = false;
bool parallel_comm = false;

inline void parse_args(int argc, char* argv[]) {
    int arg_num = 1;

    const std::string comm_create_mode = "-c";
    const std::string direct = "direct";
    const std::string reverse = "reverse";
    const std::string rank_order_mode = "-o";
    const std::string reorder = "reorder";
    const std::string parallel_comm_mode = "-p";

    while (arg_num + 1 < argc) {
        if (!comm_create_mode.compare(argv[arg_num])) {
            arg_num++;
            if (!direct.compare(argv[arg_num])) {
                for (size_t i = 0; i < comm_sizes.size(); i++) {
                    comm_sizes[i] = i + 1;
                }
            }
            else if (!reverse.compare(argv[arg_num])) {
                for (size_t i = 0; i < comm_sizes.size(); i++) {
                    comm_sizes[i] = comm_sizes.size() - i;
                }
            }
            else {
                cout << "Wrong comm create mode:" << argv[arg_num] << std::endl;
                cout << "Choose mode: direct or reverse" << std::endl;
                cout << "Exampe: mpirun -n 2 ./multi_comms -c reverse" << std::endl;
                exit(1);
            }
            arg_num++;
        }
        else if (!rank_order_mode.compare(argv[arg_num])) {
            arg_num++;
            if (!direct.compare(argv[arg_num])) {
                reorder_ranks = false;
            }
            else if (!reorder.compare(argv[arg_num])) {
                reorder_ranks = true;
            }
            else {
                cout << "Wrong rank order mode:" << argv[arg_num] << std::endl;
                cout << "Choose mode: direct or reorder" << std::endl;
                cout << "Exampe: mpirun -n 2 ./multi_comms -o direct" << std::endl;
                exit(1);
            }
            arg_num++;
        }
        else if (!parallel_comm_mode.compare(argv[arg_num])) {
            arg_num++;
            if (strstr(argv[arg_num], "0")) {
                parallel_comm = false;
            }
            else if (strstr(argv[arg_num], "1")) {
                parallel_comm = true;
            }
            else {
                cout << "Wrong multi comms mode:" << argv[arg_num] << std::endl;
                cout << "Choose mode: 0 or 1" << std::endl;
                cout << "Exampe: mpirun -n 2 ./multi_comms -p 1" << std::endl;
                exit(1);
            }
            arg_num++;
        }
    }

    cout << "arguments: comm_sizes: " << utils::vec_to_string(comm_sizes)
         << ", reorder_ranks: " << reorder_ranks << ", parallel_comm: " << parallel_comm << "\n";
}

int main(int argc, char* argv[]) {
    std::list<size_t> elem_counts = { 17, 1024, 262145 };

    int mpi_size = 0;
    int mpi_rank = 0;
    ;
    int ccl_rank = 0;
    int root = 0;
    int iter_idx = -1;

    int fail_counter = 0;

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    atexit(mpi_finalize);

    sycl::property_list props{ sycl::property::queue::in_order{} };
    queue q;

    if (!create_sycl_queue("gpu", mpi_rank, q, props)) {
        return -1;
    }

    std::vector<ccl::shared_ptr_class<ccl::kvs>> kvses;
    std::vector<ccl::communicator> comms;
    comm_sizes.resize(mpi_size);
    for (size_t i = 0; i < comm_sizes.size(); i++) {
        comm_sizes[i] = i + 1;
    }

    parse_args(argc, argv);

    ccl_rank = mpi_rank;
    for (int comm_size : comm_sizes) {
        iter_idx++;
        cout << "check comm_size: " << comm_size << "\n";

        /* create kvs */
        ccl::shared_ptr_class<ccl::kvs> kvs;
        ccl::kvs::address_type main_addr;
        MPI_Comm new_comm;
        MPI_Comm_split(MPI_COMM_WORLD, mpi_rank / comm_size, mpi_rank, &new_comm);
        int local_mpi_rank;
        MPI_Comm_rank(new_comm, &local_mpi_rank);
        int bcast_root = iter_idx % comm_size;

        int threshold = parallel_comm ? ((int)(mpi_size / comm_size)) * comm_size : comm_size;
        if (mpi_rank >= threshold) {
            continue;
        }
        if (local_mpi_rank == bcast_root) {
            kvs = ccl::create_main_kvs();
            main_addr = kvs->get_address();
            MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, bcast_root, new_comm);
        }
        else {
            MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, bcast_root, new_comm);

            kvs = ccl::create_kvs(main_addr);
            main_addr = kvs->get_address();
        }

        MPI_Comm_free(&new_comm);

        if (reorder_ranks) {
            ccl_rank = (mpi_rank + iter_idx) % comm_size;
        }
        else {
            ccl_rank = mpi_rank % comm_size;
        }

        kvses.push_back(kvs);

        /* create communicator */
        auto dev = ccl::create_device(q.get_device());
        auto ctx = ccl::create_context(q.get_context());

        cout << "before create_communicator: comm_size: " << comm_size
             << ", comm_rank: " << ccl_rank << "\n";
        comms.push_back(ccl::create_communicator(comm_size, ccl_rank, dev, ctx, kvs));
        cout << "after create_communicator: comm_size: " << comm_size << ", comm_rank: " << ccl_rank
             << "\n";

        if (comm_size != comms.back().size()) {
            cout << "FAILED unexpected comm_size " << comms.back().size() << "\n";
            fail_counter++;
        }

        /* create stream */
        auto stream = ccl::create_stream(q);

        for (auto elem_count : elem_counts) {
            cout << "check elem_count: " << elem_count << "\n";

            /* create buffers */
            auto allgatherv_send_buf = sycl::malloc_device<int>(elem_count, q);
            auto allgatherv_recv_buf = sycl::malloc_device<int>(elem_count * comm_size, q);
            std::vector<size_t> allgatherv_recv_counts(comm_size, elem_count);

            auto allreduce_send_buf = sycl::malloc_device<int>(elem_count, q);
            auto allreduce_recv_buf = sycl::malloc_device<int>(elem_count, q);

            auto alltoallv_send_buf = sycl::malloc_device<int>(elem_count * comm_size, q);
            auto alltoallv_recv_buf = sycl::malloc_device<int>(elem_count * comm_size, q);
            std::vector<size_t> alltoallv_send_counts(comm_size, elem_count);
            std::vector<size_t> alltoallv_recv_counts(comm_size, elem_count);

            auto bcast_buf = sycl::malloc_device<int>(elem_count, q);

            /* open buffers and modify them on the device side */
            auto e = q.submit([&](auto& h) {
                h.parallel_for(elem_count, [=](auto id) {
                    allgatherv_send_buf[id] = ccl_rank + id + 1;
                    for (int i = 0; i < comm_size; i++) {
                        allgatherv_recv_buf[id * comm_size + i] = -1;
                    }

                    allreduce_send_buf[id] = ccl_rank + id + 1;
                    allreduce_recv_buf[id] = -1;

                    for (int i = 0; i < comm_size; i++) {
                        alltoallv_send_buf[id * comm_size + i] = ccl_rank + 1;
                        alltoallv_recv_buf[id * comm_size + i] = -1;
                    }

                    if (ccl_rank == root) {
                        bcast_buf[id] = id + 1;
                    }
                    else {
                        bcast_buf[id] = -1;
                    }
                });
            });

            e.wait();

            int check_sum = 0;
            for (int i = 1; i <= comm_size; ++i) {
                check_sum += i;
            }

            /* do not wait completion of kernel and provide it as dependency for operation */
            vector<ccl::event> deps;
            deps.push_back(ccl::create_event(e));

            /* invoke allgatherv */
            auto allgatherv_attr = ccl::create_operation_attr<ccl::allgatherv_attr>();

            cout << "before allgatherv, comm_size: " << comm_size << "\n";
            ccl::allgatherv(allgatherv_send_buf,
                            elem_count,
                            allgatherv_recv_buf,
                            allgatherv_recv_counts,
                            comms.back(),
                            stream,
                            allgatherv_attr,
                            deps)
                .wait();
            cout << "after allgatherv, comm_size: " << comm_size << "\n";

            /* invoke allreduce */
            auto allreduce_attr = ccl::create_operation_attr<ccl::allreduce_attr>();

            cout << "before allreduce, comm_size: " << comm_size << "\n";
            ccl::allreduce(allreduce_send_buf,
                           allreduce_recv_buf,
                           elem_count,
                           ccl::reduction::sum,
                           comms.back(),
                           stream,
                           allreduce_attr,
                           deps)
                .wait();
            cout << "after allreduce, comm_size: " << comm_size << "\n";

            /* invoke alltoallv */
            auto alltoallv_attr = ccl::create_operation_attr<ccl::alltoallv_attr>();

            cout << "before alltoallv, comm_size: " << comm_size << "\n";
            ccl::alltoallv(alltoallv_send_buf,
                           alltoallv_send_counts,
                           alltoallv_recv_buf,
                           alltoallv_recv_counts,
                           comms.back(),
                           stream,
                           alltoallv_attr,
                           deps)
                .wait();
            cout << "after alltoallv, comm_size: " << comm_size << "\n";

            /* invoke bcast */
            cout << "before bcast, comm_size: " << comm_size << "\n";
            ccl::broadcast(bcast_buf, elem_count, root, comms.back(), stream).wait();
            cout << "after bcast, comm_size: " << comm_size << "\n";

            /* open result buffers and check their correctness on the device side */
            buffer<int> allgatherv_check_buf(elem_count * comm_size);
            buffer<int> allreduce_check_buf(elem_count);
            buffer<int> alltoallv_check_buf(elem_count * comm_size);
            buffer<int> bcast_check_buf(elem_count);
            q.submit([&](auto& h) {
                accessor allreduce_check_buf_acc(allreduce_check_buf, h, write_only);
                accessor bcast_check_buf_acc(bcast_check_buf, h, write_only);
                h.parallel_for(elem_count, [=](auto id) {
                    if (allreduce_recv_buf[id] != static_cast<int>(check_sum + comm_size * id)) {
                        allreduce_check_buf_acc[id] = -1;
                    }
                    else {
                        allreduce_check_buf_acc[id] = 0;
                    }

                    if (bcast_buf[id] != static_cast<int>(id + 1)) {
                        bcast_check_buf_acc[id] = -1;
                    }
                    else {
                        bcast_check_buf_acc[id] = 0;
                    }
                });
            });
            q.submit([&](auto& h) {
                 sycl::accessor allgatherv_check_buf_acc(allgatherv_check_buf, h, sycl::write_only);
                 sycl::accessor alltoallv_check_buf_acc(alltoallv_check_buf, h, sycl::write_only);
                 h.parallel_for(elem_count, [=](auto id) {
                     for (int i = 0; i < comm_size; i++) {
                         if (allgatherv_recv_buf[i * elem_count + id] !=
                             static_cast<int>(i + id + 1)) {
                             allgatherv_check_buf_acc[i * elem_count + id] = -1;
                         }
                         else {
                             allgatherv_check_buf_acc[i * elem_count + id] = 0;
                         }
                         if (alltoallv_recv_buf[i * elem_count + id] != static_cast<int>(i + 1)) {
                             alltoallv_check_buf_acc[i * elem_count + id] = -1;
                         }
                         else {
                             alltoallv_check_buf_acc[i * elem_count + id] = 0;
                         }
                     }
                 });
             }).wait();

            if (!handle_exception(q)) {
                return -1;
            }

            /* print out the result of the test on the host side */
            {
                host_accessor allgatherv_check_buf_acc(allgatherv_check_buf, read_only);
                host_accessor allreduce_check_buf_acc(allreduce_check_buf, read_only);
                host_accessor alltoallv_check_buf_acc(alltoallv_check_buf, read_only);
                host_accessor bcast_check_buf_acc(bcast_check_buf, read_only);
                size_t i;
                for (i = 0; i < elem_count; i++) {
                    int j;
                    for (j = 0; j < comm_size; j++) {
                        if (allgatherv_check_buf_acc[i * comm_size + j] == -1) {
                            cout << "FAILED allgatherv at idx " << i * comm_size + j << "\n";
                            fail_counter++;
                            break;
                        }
                    }
                    if (j != comm_size) {
                        break;
                    }
                    if (allreduce_check_buf_acc[i] == -1) {
                        cout << "FAILED allreduce at idx " << i << "\n";
                        fail_counter++;
                        break;
                    }
                    for (j = 0; j < comm_size; j++) {
                        if (alltoallv_check_buf_acc[i * comm_size + j] == -1) {
                            cout << "FAILED alltoallv at idx " << i * comm_size + j << "\n";
                            fail_counter++;
                            break;
                        }
                    }
                    if (j != comm_size) {
                        break;
                    }
                    if (bcast_check_buf_acc[i] == -1) {
                        cout << "FAILED bcast at idx " << i << "\n";
                        fail_counter++;
                        break;
                    }
                }
                if (i == elem_count) {
                    cout << "PASSED for comm_size " << comm_size << " and elem_count " << elem_count
                         << "\n";
                }
            }

            sycl::free(allgatherv_send_buf, q);
            sycl::free(allgatherv_recv_buf, q);
            sycl::free(allreduce_send_buf, q);
            sycl::free(allreduce_recv_buf, q);
            sycl::free(alltoallv_send_buf, q);
            sycl::free(alltoallv_recv_buf, q);
            sycl::free(bcast_buf, q);
        }
    }

    comms.clear();
    kvses.clear();

    if (fail_counter) {
        cout << "FAILED, counter " << fail_counter << "\n";
        return -1;
    }
    else {
        cout << "PASSED\n";
        return 0;
    }

    return 0;
}
