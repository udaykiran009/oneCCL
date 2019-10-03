#include "base.hpp"

void run_collective(const char* cmd_name,
                    const std::vector<float>& send_buf,
                    std::vector<float>& recv_buf,
                    ccl::communicator_t& comm,
                    ccl::stream_t& stream,
                    ccl::coll_attr& coll_attr)
{
    std::chrono::system_clock::duration exec_time{0};
    float expected = (comm->size() - 1) * (static_cast<float>(comm->size()) / 2);
    float received;

    comm->barrier(stream);

    for (size_t idx = 0; idx < ITERS; ++idx)
    {
        auto start = std::chrono::system_clock::now();
        comm->reduce(send_buf.data(),
                    recv_buf.data(),
                    recv_buf.size(),
                    ccl::reduction::sum,
                    COLL_ROOT,
                    &coll_attr,
                    stream)->wait();
        exec_time += std::chrono::system_clock::now() - start;
    }

    for (size_t idx = 0; idx < recv_buf.size(); idx++)
    {
        received = recv_buf[idx];
        if ((comm->rank() == COLL_ROOT) && (received != expected))
        {
            fprintf(stderr, "idx %zu, expected %4.4f, got %4.4f\n",
                    idx, expected, received);
            printf("FAILED\n");
            std::terminate();
        }
    }

    comm->barrier(stream);

    printf("avg time of %s: %lu us\n", cmd_name,
           std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count() / ITERS);
}

int main()
{
    auto comm = ccl::environment::instance().create_communicator();
    auto stream = ccl::environment::instance().create_stream();
    ccl::coll_attr coll_attr{};

    MSG_LOOP(
        std::vector<float> send_buf(msg_count, static_cast<float>(comm->rank()));
        std::vector<float> recv_buf(msg_count);
        coll_attr.to_cache = 0;
        run_collective("warmup_reduce", send_buf, recv_buf, comm, stream, coll_attr);
        coll_attr.to_cache = 1;
        run_collective("persistent_reduce", send_buf, recv_buf, comm, stream, coll_attr);
        coll_attr.to_cache = 0;
        run_collective("regular_reduce", send_buf, recv_buf, comm, stream, coll_attr);
    );

    return 0;
}
