#include "base.hpp"

void run_collective(const char* cmd_name,
                    std::vector<float>& send_buf,
                    std::vector<float>& recv_buf,
                    std::vector<size_t>& recv_counts,
                    ccl::communicator_t& comm,
                    ccl::stream_t& stream,
                    ccl::coll_attr& coll_attr) {
    std::chrono::system_clock::duration exec_time{ 0 };
    float expected = send_buf.size();
    float received;

    comm->barrier(stream);

    for (size_t idx = 0; idx < ITERS; ++idx) {
        auto start = std::chrono::system_clock::now();
        comm->allgatherv(send_buf.data(),
                         send_buf.size(),
                         recv_buf.data(),
                         recv_counts.data(),
                         &coll_attr,
                         stream)
            ->wait();
        exec_time += std::chrono::system_clock::now() - start;
    }

    for (size_t idx = 0; idx < recv_buf.size(); idx++) {
        received = recv_buf[idx];
        if (received != expected) {
            fprintf(stderr, "idx %zu, expected %4.4f, got %4.4f\n", idx, expected, received);

            std::cout << "FAILED" << std::endl;
            std::terminate();
        }
    }

    comm->barrier(stream);

    std::cout << "avg time of " << cmd_name << ": "
              << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count() / ITERS
              << ", us" << std::endl;
}

int main() {
    auto comm = ccl::environment::instance().create_communicator();
    auto stream = ccl::environment::instance().create_stream();
    ccl::coll_attr coll_attr{};

    MSG_LOOP(comm, std::vector<float> send_buf(msg_count, static_cast<float>(msg_count));
             std::vector<float> recv_buf(comm->size() * msg_count, 0);
             std::vector<size_t> recv_counts(comm->size(), msg_count);
             coll_attr.to_cache = 0;
             run_collective(
                 "warmup_allgatherv", send_buf, recv_buf, recv_counts, comm, stream, coll_attr);
             coll_attr.to_cache = 1;
             run_collective(
                 "persistent_allgatherv", send_buf, recv_buf, recv_counts, comm, stream, coll_attr);
             coll_attr.to_cache = 0;
             run_collective(
                 "regular_allgatherv", send_buf, recv_buf, recv_counts, comm, stream, coll_attr););

    return 0;
}
