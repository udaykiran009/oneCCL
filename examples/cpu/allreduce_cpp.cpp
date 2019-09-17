#include "base.hpp"

void run_collective(const char* cmd_name,
                    const std::vector<float>& send_buf,
                    std::vector<float>& recv_buf,
                    ccl::communicator& comm,
                    ccl::stream& stream,
                    ccl::coll_attr& coll_attr)
{
    std::chrono::system_clock::duration exec_time{0};
    float expected = (comm.size() - 1) * (static_cast<float>(comm.size()) / 2);

    comm.barrier(&stream);

    for (size_t idx = 0; idx < ITERS; ++idx)
    {
        auto start = std::chrono::system_clock::now();
        comm.allreduce(send_buf.data(),
                       recv_buf.data(),
                       recv_buf.size(),
                       ccl::data_type::dt_float,
                       ccl::reduction::sum,
                       &coll_attr,
                       &stream)->wait();
        exec_time += std::chrono::system_clock::now() - start;
    }

    for (size_t idx = 0; idx < recv_buf.size(); idx++)
    {
        if (recv_buf[idx] != expected)
        {
            fprintf(stderr, "idx %zu, expected %4.4f, got %4.4f\n",
                    idx, expected, recv_buf[idx]);
            printf("FAILED\n");
            std::terminate();
        }
    }

    comm.barrier(&stream);

    printf("avg time of %s: %lu us\n", cmd_name,
           std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count() / ITERS);
}

int main()
{
    ccl::environment env;
    ccl::communicator comm;
    ccl::stream stream;
    ccl::coll_attr coll_attr{};

    MSG_LOOP(
        std::vector<float> send_buf(msg_count, static_cast<float>(comm.rank()));
        std::vector<float> recv_buf(msg_count);
        coll_attr.to_cache = 0;
        run_collective("warmup allreduce", send_buf, recv_buf, comm, stream, coll_attr);
        coll_attr.to_cache = 1;
        run_collective("persistent allreduce", send_buf, recv_buf, comm, stream, coll_attr);
        coll_attr.to_cache = 0;
        run_collective("regular allreduce", send_buf, recv_buf, comm, stream, coll_attr);
    );

    return 0;
}
