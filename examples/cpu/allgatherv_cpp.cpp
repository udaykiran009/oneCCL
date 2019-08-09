
#include "base.hpp"

void run_collective(const char* cmd_name,
                    std::vector<float>& send_buf,
                    const size_t count,
                    float* recv_buf,
                    size_t *recv_counts,
                    ccl::communicator& comm,
                    ccl::stream* stream,
                    ccl_coll_attr_t& coll_attr)
{
    std::chrono::system_clock::duration exec_time{};
    float expected = (comm.size() - 1) * (static_cast<float>(comm.size()) / 2);

    comm.barrier(stream);

    for (size_t idx = 0; idx < ITERS; ++idx)
    {
        float expected = 1.0;
        for (idx = 0; idx < COUNT; idx++)
            send_buf[idx] = expected;
        for (idx = 0; idx < comm.size() * COUNT; idx++)
            recv_buf[idx] = 0.0;
        auto start = std::chrono::system_clock::now();
        comm.allgatherv(send_buf.data(),
                       COUNT,
                       recv_buf,
                       recv_counts,
                       ccl::data_type::dtype_float,
                       &coll_attr,
                       stream)->wait();

        exec_time += std::chrono::system_clock::now() - start;
    }

    for (size_t recv_idx = 0; recv_idx < comm.size() * COUNT; recv_idx++)    
    {
        if (recv_buf[recv_idx] != expected)
        {
            fprintf(stderr, "recv_idx %zu, expected %4.4f, got %4.4f\n",
                    recv_idx, expected, recv_buf[recv_idx]);
            std::terminate();
        }
    }

    comm.barrier(stream);
    printf("avg time of %s: %lu us\n", cmd_name,
           std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count() / ITERS);
}

int main()
{
    std::vector<size_t> msg_counts(MSG_SIZE_COUNT);

    for (size_t idx = 0; idx < MSG_SIZE_COUNT; ++idx)
    {
        msg_counts[idx] = 1u << (START_MSG_SIZE_POWER + idx);
    }

    ccl_coll_attr_t coll_attr{};

    try
    {

        for (auto msg_count : msg_counts)
        {
            float *recv_buf;
            size_t *recv_counts;
            std::vector<float> send_buf(msg_count, static_cast<float>(COUNT));
            recv_buf = static_cast<float*>(malloc(comm.size() * COUNT * sizeof(float)));
            recv_counts = static_cast<size_t*>(malloc(comm.size() * sizeof(size_t)));
            for (size_t idx = 0; idx < comm.size(); idx++)
                recv_counts[idx] = COUNT;

            coll_attr.to_cache = 0;
            run_collective("regular allgatherv", send_buf, COUNT, recv_buf, recv_counts, comm, &stream, coll_attr);
            PRINT_BY_ROOT("PASSED");
        }
    }
    catch (ccl::ccl_error& e)
    {
        fprintf(stderr, "ccl exception:\n%s\n", e.what());
        return -1;
    }
    catch (...)
    {
        fprintf(stderr, "other exception\n");
        return -1;
    }
    return 0;
}
