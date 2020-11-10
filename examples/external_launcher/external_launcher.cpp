#include <stdlib.h>

#include "oneapi/ccl.hpp"
#include "store.hpp"

#define ITER_COUNT   10
#define REINIT_COUNT 10

void run_collective(const char* cmd_name,
                    const std::vector<float>& send_buf,
                    std::vector<float>& recv_buf,
                    const ccl::communicator& comm) {
    std::chrono::system_clock::duration exec_time{ 0 };
    float expected = (comm.size() - 1) * (static_cast<float>(comm.size()) / 2);

    ccl::barrier(comm);

    for (size_t idx = 0; idx < ITER_COUNT; ++idx) {
        auto start = std::chrono::system_clock::now();
        ccl::allreduce(send_buf.data(), recv_buf.data(), recv_buf.size(), ccl::reduction::sum, comm)
            .wait();
        exec_time += std::chrono::system_clock::now() - start;
    }

    for (size_t idx = 0; idx < recv_buf.size(); idx++) {
        if (recv_buf[idx] != expected) {
            fprintf(stderr, "idx %zu, expected %4.4f, got %4.4f\n", idx, expected, recv_buf[idx]);

            std::cout << "FAILED" << std::endl;
            std::terminate();
        }
    }

    ccl::barrier(comm);

    std::cout << "avg time of " << cmd_name << ": "
              << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count() /
                     ITER_COUNT
              << ", us\n"
              << std::flush;
}

void print_help() {
    std::cout << "specify: [size] [rank] [store_path]"
              << "\n";
}

int main(int argc, char** argv) {
    const size_t elem_count = 64 * 1024;
    int ret = 0;
    int size, rank;
    char* path;

    if (argc == 4) {
        size = std::atoi(argv[1]);
        rank = std::atoi(argv[2]);
        path = argv[3];

        std::cout << "[" << rank << "] args: "
                  << "size = " << size << ", rank = " << rank << "store path = " << path << "\n";
    }
    else {
        print_help();
        return -1;
    }

    ccl::init();

    for (int i = 0; i < REINIT_COUNT; ++i) {
        std::cout << "[" << rank << "] started iter " << i << "\n" << std::flush;

        std::chrono::system_clock::duration exec_time{ 0 };
        auto start = std::chrono::system_clock::now();

        ccl::shared_ptr_class<ccl::kvs> kvs;
        ccl::kvs::address_type main_addr;

        auto store = std::make_shared<file_store>(path, rank, std::chrono::seconds(120));

        if (rank == 0) {
            kvs = ccl::create_main_kvs();
            main_addr = kvs->get_address();
            exec_time = std::chrono::system_clock::now() - start;
            std::cout << "main kvs create time: "
                      << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count()
                      << ", us\n"
                      << std::flush;

            start = std::chrono::system_clock::now();
            if (store->write((void*)main_addr.data(), main_addr.size()) < 0) {
                printf("An error occurred during write attempt\n");
                return -1;
            }
            exec_time = std::chrono::system_clock::now() - start;
            std::cout << "write to store time: "
                      << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count()
                      << ", us\n"
                      << std::flush;
        }
        else {
            if (store->read((void*)main_addr.data(), main_addr.size()) < 0) {
                printf("An error occurred during read attempt\n");
                return -1;
            }
            exec_time = std::chrono::system_clock::now() - start;
            std::cout << "read from store time: "
                      << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count()
                      << ", us\n"
                      << std::flush;

            start = std::chrono::system_clock::now();
            kvs = ccl::create_kvs(main_addr);
            exec_time = std::chrono::system_clock::now() - start;
            std::cout << "kvs create time: "
                      << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count()
                      << ", us\n"
                      << std::flush;
        }

        start = std::chrono::system_clock::now();
        auto comm = ccl::create_communicator(size, rank, kvs);
        exec_time = std::chrono::system_clock::now() - start;
        std::cout << "communicator create time: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count()
                  << ", us\n"
                  << std::flush;

        start = std::chrono::system_clock::now();
        std::vector<float> send_buf(elem_count, static_cast<float>(comm.rank()));
        std::vector<float> recv_buf(elem_count);
        run_collective("allreduce", send_buf, recv_buf, comm);
        exec_time = std::chrono::system_clock::now() - start;
        std::cout << "total allreduce time: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count()
                  << ", us\n"
                  << std::flush;

        ccl::barrier(comm);

        if (rank == 0)
            store.reset();

        ccl::barrier(comm);

        std::cout << "[" << rank << "] completed iter " << i << "\n" << std::flush;
    }

    std::cout << "[" << rank << "] PASSED\n" << std::flush;

    return ret;
}
