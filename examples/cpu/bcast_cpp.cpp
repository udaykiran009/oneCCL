
#include "base.hpp"

void run_collective(const char* cmd_name,
                    std::vector<float>& send_buf,
                    std::vector<float>& recv_buf,
                    ccl::communicator& comm,
                    ccl::stream* stream,
                    ccl_coll_attr_t& coll_attr)
{
    std::chrono::system_clock::duration exec_time{};
    float expected =static_cast<float>(comm.rank());

    comm.barrier(stream);

    for (size_t idx = 0; idx < ITERS; ++idx)
    {
        for (idx = 0; idx < send_buf.size(); idx++)
        {
            if (comm.rank() == 0)
                send_buf[idx] = idx;
            else 
                send_buf[idx] = 0.0;
        }
            auto start = std::chrono::system_clock::now();
            comm.bcast(send_buf.data(),
                           send_buf.size(),
                           ccl::data_type::dtype_float,
                           0,
                           &coll_attr, 
                           stream)->wait();

            exec_time += std::chrono::system_clock::now() - start;
        }

    for (size_t recv_idx = 0; recv_idx < send_buf.size(); ++recv_idx)
    {
        if (send_buf[recv_idx] != recv_idx)
        {
            fprintf(stderr, "idx %zu, expected %4.4f, got %4.4f\n",
                    recv_idx, expected, send_buf[recv_idx]);
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
            std::vector<float> recv_buf(msg_count);
            std::vector<float> send_buf(msg_count, static_cast<float>(comm.rank()));

            coll_attr.to_cache = 0;
            run_collective("regular bcast", send_buf, recv_buf, comm, &stream, coll_attr);
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
