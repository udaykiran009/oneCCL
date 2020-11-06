#include <stdlib.h>

#include "base.hpp"
#include "store.hpp"

#define REINIT 10
#define REPEAT 10

void run_collective(const char* cmd_name,
                    const std::vector<float>& send_buf,
                    std::vector<float>& recv_buf,
                    const ccl::communicator& comm,
                    const ccl::allreduce_attr& attr) {
    std::chrono::system_clock::duration exec_time{ 0 };
    float expected = (comm.size() - 1) * (static_cast<float>(comm.size()) / 2);

    ccl::barrier(comm);

    for (size_t idx = 0; idx < ITERS; ++idx) {
        auto start = std::chrono::system_clock::now();
        ccl::allreduce(
            send_buf.data(), recv_buf.data(), recv_buf.size(), ccl::reduction::sum, comm, attr)
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
              << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count() / ITERS
              << ", us" << std::endl;
}

void print_help() {
    printf("You must use this example like:\n"
           "launcher world_size rank kvs_file_store_path\n");
}

int main(int argc, char** argv) {
    int ret = 0;
    int size, rank;
    char* path;

    if (argc == 4) {
        size = std::atoi(argv[1]);
        rank = std::atoi(argv[2]);
        path = argv[3];
        printf("Received arguments: world_size=%d, rank=%d, key-value file store path=%s\n",
               size,
               rank,
               path);
    }
    else {
        print_help();
        return -1;
    }

    ccl::init();

    for (int i = 0; i < REINIT; ++i) {
        ccl::shared_ptr_class<ccl::kvs> kvs;
        ccl::kvs::address_type main_addr;
        auto store = std::make_shared<file_store>(path, std::chrono::seconds(120));
        if (rank == 0) {
            kvs = ccl::create_main_kvs();
            main_addr = kvs->get_address();
            if (store->write((void*)main_addr.data(), main_addr.size()) < 0) {
                printf("An error occurred during write attempt\n");
                return -1;
            }
        }
        else {
            if (store->read((void*)main_addr.data(), main_addr.size()) < 0) {
                printf("An error occurred during read attempt\n");
                return -1;
            }
            kvs = ccl::create_kvs(main_addr);
        }
        auto comm = ccl::create_communicator(size, rank, kvs);
        auto attr = ccl::create_operation_attr<ccl::allreduce_attr>();

        for (int j = 0; j < REPEAT; ++j) {
            MSG_LOOP(comm, std::vector<float> send_buf(msg_count, static_cast<float>(comm.rank()));
                     std::vector<float> recv_buf(msg_count);
                     run_collective("regular allreduce", send_buf, recv_buf, comm, attr););
        }
    }

    if (rank == 0)
        printf("PASSED\n");

    return ret;
}
