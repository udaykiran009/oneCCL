#include "sycl_base.hpp"

using namespace std;
using namespace sycl;

bool reorder_ranks = false;
bool multi_comm = false;
std::vector<int> comm_sizes{};

inline void parcer(int argc, char* argv[]) {
    int arg_num = 1;
    const std::string comm_create_mode = "-c";
    const std::string direct = "direct";
    const std::string reverse = "reverse";
    const std::string ranks_ordering_mode = "-o";
    const std::string reorder = "reorder";
    const std::string multi_comms_mode = "-m";
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
        else if (!ranks_ordering_mode.compare(argv[arg_num])) {
            arg_num++;
            if (!direct.compare(argv[arg_num])) {
                reorder_ranks = false;
            }
            else if (!reorder.compare(argv[arg_num])) {
                reorder_ranks = true;
            }
            else {
                cout << "Wrong ranks ordering mode:" << argv[arg_num] << std::endl;
                cout << "Choose mode: direct or reorder" << std::endl;
                cout << "Exampe: mpirun -n 2 ./multi_comms -o direct" << std::endl;
                exit(1);
            }
            arg_num++;
        }
        else if (!multi_comms_mode.compare(argv[arg_num])) {
            arg_num++;
            if (strstr(argv[arg_num], "0")) {
                multi_comm = false;
            }
            else if (strstr(argv[arg_num], "1")) {
                multi_comm = true;
            }
            else {
                cout << "Wrong multi comms mode:" << argv[arg_num] << std::endl;
                cout << "Choose mode: 0 or 1" << std::endl;
                cout << "Exampe: mpirun -n 2 ./multi_comms -m 1" << std::endl;
                exit(1);
            }
            arg_num++;
        }
    }
}

int main(int argc, char* argv[]) {
    std::list<size_t> elem_counts = { 17, 1024, 262145 };

    int size = 0;
    int mpi_rank = 0;
    ;
    int ccl_rank = 0;
    int root = 0;
    int iter_idx = -1;

    int fail_counter = 0;

    sycl::property_list props{ sycl::property::queue::in_order{} };
    queue q{ props };

    if (!q.get_device().is_gpu()) {
        cout << "test expects GPU device, please use SYCL_DEVICE_FILTER accordingly";
        return -1;
    }

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    atexit(mpi_finalize);

    std::vector<ccl::shared_ptr_class<ccl::kvs>> kvses;
    std::vector<ccl::communicator> comms;
    comm_sizes.resize(size);
    for (size_t i = 0; i < comm_sizes.size(); i++) {
        comm_sizes[i] = i + 1;
    }
    parcer(argc, argv);

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

        int threshold = multi_comm ? ((int)(size / comm_size)) * comm_size : comm_size;
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

        cout << "before create_communicator for comm_size " << comm_size << "\n";
        comms.push_back(ccl::create_communicator(comm_size, ccl_rank, dev, ctx, kvs));
        cout << "after create_communicator for comm_size " << comm_size << "\n";

        if (comm_size != comms.back().size()) {
            cout << "FAILED unexpected comm_size " << comms.back().size() << "\n";
            fail_counter++;
        }

        /* create stream */
        auto stream = ccl::create_stream(q);

        for (auto elem_count : elem_counts) {
            cout << "check elem_count: " << elem_count << "\n";

            /* create buffers */
            auto send_buf = sycl::malloc_device<int>(elem_count, q);
            auto recv_buf = sycl::malloc_device<int>(elem_count, q);
            auto buf = sycl::malloc_device<int>(elem_count, q);

            /* open buffers and modify them on the device side */
            auto e = q.submit([&](auto& h) {
                h.parallel_for(elem_count, [=](auto id) {
                    send_buf[id] = ccl_rank + id + 1;
                    recv_buf[id] = -1;
                    if (ccl_rank == root) {
                        buf[id] = id + 1;
                    }
                    else {
                        buf[id] = -1;
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

            /* invoke allreduce */
            auto attr = ccl::create_operation_attr<ccl::allreduce_attr>();

            cout << "before allreduce for comm_size " << comm_size << "\n";
            ccl::allreduce(send_buf,
                           recv_buf,
                           elem_count,
                           ccl::reduction::sum,
                           comms.back(),
                           stream,
                           attr,
                           deps)
                .wait();
            cout << "after allreduce for comm_size " << comm_size << "\n";

            cout << "before bcast for comm_size " << comm_size << "\n";
            ccl::broadcast(buf, elem_count, root, comms.back(), stream).wait();
            cout << "after bcast for comm_size " << comm_size << "\n";

            /* open result buffers and check their correctness on the device side */
            buffer<int> allreduce_check_buf(elem_count);
            buffer<int> bcast_check_buf(elem_count);
            q.submit([&](auto& h) {
                accessor allreduce_check_buf_acc(allreduce_check_buf, h, write_only);
                accessor bcast_check_buf_acc(bcast_check_buf, h, write_only);
                h.parallel_for(elem_count, [=](auto id) {
                    if (recv_buf[id] != static_cast<int>(check_sum + comm_size * id)) {
                        allreduce_check_buf_acc[id] = -1;
                    }
                    else {
                        allreduce_check_buf_acc[id] = 0;
                    }

                    if (buf[id] != static_cast<int>(id + 1)) {
                        bcast_check_buf_acc[id] = -1;
                    }
                    else {
                        bcast_check_buf_acc[id] = 0;
                    }
                });
            });

            if (!handle_exception(q)) {
                return -1;
            }

            /* print out the result of the test on the host side */
            {
                host_accessor allreduce_check_buf_acc(allreduce_check_buf, read_only);
                host_accessor bcast_check_buf_acc(bcast_check_buf, read_only);
                size_t i;
                for (i = 0; i < elem_count; i++) {
                    if (allreduce_check_buf_acc[i] == -1) {
                        cout << "FAILED allreduce at idx " << i << "\n";
                        fail_counter++;
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

            sycl::free(send_buf, q);
            sycl::free(recv_buf, q);
            sycl::free(buf, q);
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
