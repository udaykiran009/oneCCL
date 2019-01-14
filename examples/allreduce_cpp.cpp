
#include "mlsl.hpp"

#include <functional>
#include <vector>
#include <chrono>

const size_t COUNT = 1048576;
const size_t ITERS = 128;

void run_collective(const std::function<std::shared_ptr<mlsl::request>()>& start_cmd,
                    const char* cmd_name,
                    std::vector<float>& recv_buf,
                    mlsl::communicator& comm)
{
    std::chrono::system_clock::duration exec_time{};
    float expected = (comm.size() - 1) * (static_cast<float>(comm.size()) / 2);
    printf("Running %s\n", cmd_name);

    for (size_t idx = 0; idx < ITERS; ++idx)
    {
        std::fill(recv_buf.begin(), recv_buf.end(), static_cast<float>(comm.rank()));
        auto start = std::chrono::system_clock::now();
        start_cmd()->wait();
        exec_time += std::chrono::system_clock::now() - start;

        //check result after every iteration
        for (size_t recv_idx = 0; recv_idx < recv_buf.size(); ++recv_idx)
        {
            if (recv_buf[recv_idx] != expected)
            {
                fprintf(stderr, "iter %zu, idx %zu, expected %4.4f, got %4.4f\n",
                        idx, recv_idx, expected, recv_buf[recv_idx]);
                std::terminate();
            }
        }
    }

    comm.barrier();

    fprintf(stdout, "avg time of %s: %lu us\n", cmd_name,
            std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count() / ITERS);
}

int main()
{
    mlsl::environment env;
    mlsl::communicator global_comm;

    std::vector<float> recv_buf(COUNT);
    std::vector<float> send_buf(COUNT, static_cast<float>(global_comm.rank()));
    mlsl_coll_attr_t coll_attr{};

    auto command = [&]() -> std::shared_ptr<mlsl::request>
    {
        return global_comm.allreduce(static_cast<void*>(send_buf.data()),
                                     static_cast<void*>(recv_buf.data()),
                                     COUNT,
                                     mlsl::data_type::dtype_float,
                                     mlsl::reduction::sum,
                                     &coll_attr);
    };

    try
    {
        coll_attr.to_cache = 1;
        run_collective(command, "persistent allreduce", recv_buf, global_comm);

        coll_attr.to_cache = 0;
        run_collective(command, "regular allreduce", recv_buf, global_comm);
    }
    catch (mlsl::mlsl_error& e)
    {
        fprintf(stderr, "mlsl exception has been caught:\n%s\n", e.what());
    }
    catch (...)
    {
        fprintf(stderr, "other exception has been caught\n");
    }
    return 0;
}
