#include "base.hpp"

void run_collective(const char* cmd_name,
                    std::vector<float>& buf,
                    ccl::communicator& comm,
                    ccl::stream& stream,
                    ccl::coll_attr& coll_attr)
{
    std::chrono::system_clock::duration exec_time{0};
    float received;

    if (comm.rank() == COLL_ROOT)
    {
        for (size_t idx = 0; idx < buf.size(); idx++)
        {
            buf[idx] = static_cast<float>(idx);
        }
    }
    comm.barrier(&stream);

    for (size_t idx = 0; idx < ITERS; ++idx)
    {   
        auto start = std::chrono::system_clock::now();
        comm.bcast(buf.data(),
                   buf.size(),
                   ccl::data_type::dt_float,
                   COLL_ROOT,
                   &coll_attr,
                   &stream)->wait();
        exec_time += std::chrono::system_clock::now() - start;
    }

    for (size_t idx = 0; idx < buf.size(); idx++)
    {
        received = buf[idx];
        if (received != idx)
        {
            fprintf(stderr, "idx %zu, expected %4.4f, got %4.4f\n",
                    idx, static_cast<float>(idx), received);
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
        std::vector<float> buf(msg_count);
        coll_attr.to_cache = 0;
        run_collective("warmup_bcast", buf, comm, stream, coll_attr);
        coll_attr.to_cache = 1;
        run_collective("persistent_bcast", buf, comm, stream, coll_attr);
        coll_attr.to_cache = 0;
        run_collective("regular_bcast", buf, comm, stream, coll_attr);
    );

    return 0;
}
