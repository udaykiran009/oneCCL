
#include "ccl.hpp"

#include <functional>
#include <vector>
#include <chrono>

const size_t ITERS = 1000;

#define PRINT_BY_ROOT(fmt, ...)             \
    if(comm.rank() == 0) {                  \
        printf(fmt"\n", ##__VA_ARGS__); }   \

void run_collective(const char* cmd_name,
                    const std::vector<float>& send_buf,
                    std::vector<float>& recv_buf,
                    ccl::communicator& comm,
                    ccl_coll_attr_t& coll_attr)
{
    std::chrono::system_clock::duration exec_time{};
    float expected = (comm.size() - 1) * (static_cast<float>(comm.size()) / 2);

    comm.barrier();

    for (size_t idx = 0; idx < ITERS; ++idx)
    {
        std::fill(recv_buf.begin(), recv_buf.end(), static_cast<float>(comm.rank()));

        auto start = std::chrono::system_clock::now();
        comm.allreduce(send_buf.data(),
                       recv_buf.data(),
                       recv_buf.size(),
                       ccl::data_type::dtype_float,
                       ccl::reduction::sum,
                       &coll_attr)->wait();

        exec_time += std::chrono::system_clock::now() - start;
    }

    for (size_t recv_idx = 0; recv_idx < recv_buf.size(); ++recv_idx)
    {
        if (recv_buf[recv_idx] != expected)
        {
            fprintf(stderr, "idx %zu, expected %4.4f, got %4.4f\n",
                    recv_idx, expected, recv_buf[recv_idx]);
            std::terminate();
        }
    }

    comm.barrier();

    printf("avg time of %s: %lu us\n", cmd_name,
           std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count() / ITERS);
}

int main()
{
    size_t start_msg_size_power = 10;
    const size_t msg_size_count = 11;
    std::vector<size_t> msg_counts(msg_size_count);

    for (size_t idx = 0; idx < msg_size_count; ++idx)
    {
        msg_counts[idx] = 1u << (start_msg_size_power + idx);
    }

    ccl_coll_attr_t coll_attr{};

    try
    {
        ccl::environment env;
        ccl::communicator comm;

        for (auto msg_count : msg_counts)
        {
            PRINT_BY_ROOT("msg_size=%zu", msg_count * sizeof(float));

            std::vector<float> recv_buf(msg_count);
            std::vector<float> send_buf(msg_count, static_cast<float>(comm.rank()));

            coll_attr.to_cache = 0;
            run_collective("warmup allreduce", send_buf, recv_buf, comm, coll_attr);

            coll_attr.to_cache = 1;
            run_collective("persistent allreduce", send_buf, recv_buf, comm, coll_attr);

            coll_attr.to_cache = 0;
            run_collective("regular allreduce", send_buf, recv_buf, comm, coll_attr);
        }
    }
    catch (ccl::ccl_error& e)
    {
        fprintf(stderr, "ccl exception:\n%s\n", e.what());
    }
    catch (...)
    {
        fprintf(stderr, "other exception\n");
    }

    return 0;
}
