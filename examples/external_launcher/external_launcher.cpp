#include <stdlib.h>

#include "oneapi/ccl.hpp"
#include "store.hpp"

#define ITER_COUNT               10
#define REINIT_COUNT             3
#define KVS_CREATE_SUCCESS       0
#define STORE_READ_WRITE_TIMEOUT 120

size_t base_port = 10000;

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
            fprintf(stderr, "idx %zu, expected %4.4f, got %4.4f", idx, expected, recv_buf[idx]);

            std::cout << "FAILED" << std::endl;
            std::terminate();
        }
    }

    ccl::barrier(comm);

    std::cout << "avg time of " << cmd_name << ": "
              << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count() /
                     ITER_COUNT
              << ", us" << std::endl;
}

void print_help() {
    std::cout << "specify: [size] [rank] [store_path]" << std::endl;
}

int create_kvs_by_store(std::shared_ptr<file_store> store,
                        int rank,
                        ccl::shared_ptr_class<ccl::kvs>& kvs) {
    std::chrono::system_clock::duration exec_time{ 0 };
    auto start = std::chrono::system_clock::now();
    ccl::kvs::address_type main_addr;
    if (rank == 0) {
        kvs = ccl::create_main_kvs();
        main_addr = kvs->get_address();
        exec_time = std::chrono::system_clock::now() - start;
        std::cout << "main kvs create time: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count()
                  << ", us" << std::endl;

        start = std::chrono::system_clock::now();
        if (store->write((void*)main_addr.data(), main_addr.size()) < 0) {
            printf("An error occurred during write attempt\n");
            kvs.reset();
            return -1;
        }
        exec_time = std::chrono::system_clock::now() - start;
        std::cout << "write to store time: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count()
                  << ", us" << std::endl;
    }
    else {
        if (store->read((void*)main_addr.data(), main_addr.size()) < 0) {
            printf("An error occurred during read attempt\n");
            kvs.reset();
            return -1;
        }
        exec_time = std::chrono::system_clock::now() - start;
        std::cout << "read from store time: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count()
                  << ", us" << std::endl;

        start = std::chrono::system_clock::now();
        kvs = ccl::create_kvs(main_addr);
        exec_time = std::chrono::system_clock::now() - start;
        std::cout << "kvs create time: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count()
                  << ", us" << std::endl;
    }
    return KVS_CREATE_SUCCESS;
}

int create_kvs_by_attrs(ccl::kvs_attr kvs_attr, ccl::shared_ptr_class<ccl::kvs>& kvs) {
    std::chrono::system_clock::duration exec_time{ 0 };
    auto start = std::chrono::system_clock::now();

    kvs = ccl::create_main_kvs(kvs_attr);
    exec_time = std::chrono::system_clock::now() - start;
    std::cout << "main kvs create time: "
              << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count() << ", us"
              << std::endl;
    return KVS_CREATE_SUCCESS;
}

int main(int argc, char** argv) {
    const size_t elem_count = 64 * 1024;
    int ret = 0;
    int size, rank;
    char* path;
    std::shared_ptr<file_store> store;

    if (argc == 4) {
        size = std::atoi(argv[1]);
        rank = std::atoi(argv[2]);
        path = argv[3];

        std::cout << "[" << rank << "] args: "
                  << "size = " << size << ", rank = " << rank << "store path = " << path
                  << std::endl;
    }
    else {
        print_help();
        return -1;
    }

    ccl::init();
    auto kvs_attr = ccl::create_kvs_attr();
    char* ip = getenv("KVS_IP");
    for (int i = 0; i < REINIT_COUNT; ++i) {
        if (ip != nullptr) {
            ccl::string pi_port(std::string(ip) + "_" + std::to_string(base_port));
            kvs_attr.set<ccl::kvs_attr_id::ip_port>(pi_port);
            base_port++;
        }
        std::cout << "[" << rank << "] started iter " << i << std::endl;

        std::chrono::system_clock::duration exec_time{ 0 };

        ccl::shared_ptr_class<ccl::kvs> kvs;

        if (kvs_attr.get<ccl::kvs_attr_id::ip_port>() == "") {
            store = std::make_shared<file_store>(
                path, rank, std::chrono::seconds(STORE_READ_WRITE_TIMEOUT));
            if (create_kvs_by_store(store, rank, kvs) != KVS_CREATE_SUCCESS) {
                std::cout << "Can't to create kvs by store" << std::endl;
                return -1;
            }
        }
        else {
            if (create_kvs_by_attrs(kvs_attr, kvs) != KVS_CREATE_SUCCESS) {
                std::cout << "Can't to create kvs by attr" << std::endl;
                return -1;
            }
        }

        auto start = std::chrono::system_clock::now();
        auto comm = ccl::create_communicator(size, rank, kvs);
        exec_time = std::chrono::system_clock::now() - start;
        std::cout << "communicator create time: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count()
                  << ", us" << std::endl;

        start = std::chrono::system_clock::now();
        std::vector<float> send_buf(elem_count, static_cast<float>(comm.rank()));
        std::vector<float> recv_buf(elem_count);
        run_collective("allreduce", send_buf, recv_buf, comm);
        exec_time = std::chrono::system_clock::now() - start;
        std::cout << "total allreduce time: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count()
                  << ", us" << std::endl;

        ccl::barrier(comm);

        if (rank == 0)
            store.reset();

        ccl::barrier(comm);

        std::cout << "[" << rank << "] completed iter " << i << std::endl;
    }

    std::cout << "[" << rank << "] PASSED" << std::endl;

    return ret;
}
