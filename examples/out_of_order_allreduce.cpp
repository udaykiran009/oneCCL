#include "base.h"

#include <vector>
#include <list>
#include <string>
#include <utility>
#include <thread>
#include <random>

mlsl_request* start_allreduce_with_tensor_name(const std::string& tensor_name,
                                               const float* send_buff,
                                               float* recv_buff)
{
    coll_attr.to_cache = false;
    coll_attr.match_id = tensor_name.c_str();

    mlsl_request* req = nullptr;

    MLSL_CALL(mlsl_allreduce(send_buff,
                             recv_buff,
                             COUNT,
                             mlsl_dtype_float,
                             mlsl_reduction_sum,
                             &coll_attr,
                             nullptr,
                             &req));
    return req;
}

int main()
{
    printf("Forced out of order support\n");
    setenv("MLSL_OUT_OF_ORDER_SUPPORT", "1", 1);

    const size_t iterations_count = 4;
    std::vector<std::string> tensor_names;
    //request, operation idx (for example purpose)
    std::list<std::pair<mlsl_request*, size_t>> started_ops;
    std::vector<std::vector<float>> allreduce_send_bufs;
    std::vector<std::vector<float>> allreduce_recv_bufs;

    test_init();

    size_t comm_size = mlsl_get_comm_size(nullptr);

    std::random_device rand_dev;
    std::uniform_int_distribution<size_t> distribution(0, comm_size - 1);

    for (size_t rank_idx = 0; rank_idx < comm_size; ++rank_idx)
    {
        tensor_names.emplace_back("tensor_rank_" + std::to_string(rank_idx));
        allreduce_send_bufs.emplace_back(COUNT, rank_idx + 1);
        allreduce_recv_bufs.emplace_back(COUNT, 0.0f);
    }

    if (mlsl_get_comm_rank(nullptr) != 0)
    {
        //delay non-root ranks to check that delayed comm creation works
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    for (size_t iteration = 0; iteration < iterations_count; ++iteration)
    {
        printf("## Starting iteration #%zu\n", iteration);

        size_t start_idx = distribution(rand_dev);
        size_t rank_idx = start_idx;
        size_t operations_count = 0;

        for (; operations_count < comm_size; ++operations_count, rank_idx = (rank_idx + 1) % comm_size)
        {
            //start allreduce with shift in tensor names
            printf("   Submit allreduce #%zu for tensor %s\n", rank_idx, tensor_names[rank_idx].c_str());
            started_ops.emplace_back(start_allreduce_with_tensor_name(tensor_names[rank_idx],
                                                                      allreduce_send_bufs[rank_idx].data(),
                                                                      allreduce_recv_bufs[rank_idx].data()),
                                     rank_idx);
        }

        int test_completed = 0;

        while (!started_ops.empty())
        {
            for (auto it = started_ops.begin(); it != started_ops.end();)
            {
                mlsl_test(it->first, &test_completed);
                if (test_completed)
                {
                    float expected = (it->second + 1) * comm_size;
                    printf("   Completed allreduce #%zu for tensor %s. Actual %3.2f, expected %3.2f\n",
                           it->second, tensor_names[it->second].c_str(),
                           allreduce_recv_bufs[it->second][0],
                           expected);
                    for (size_t idx = 0; idx < COUNT; ++idx)
                    {
                        if (allreduce_recv_bufs[it->second][idx] != expected)
                        {
                            fprintf(stderr, "!! Wrong result, rank %zu, result %3.2f, intitial %3.2f, exp %3.f\n",
                                    it->second,
                                    allreduce_recv_bufs[it->second][idx],
                                    allreduce_send_bufs[it->second][idx],
                                    expected);
                            exit(1);
                        }
                    }

                    it = started_ops.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        printf("## Iteration #%zu has been finished\n", iteration);
    }

    test_finalize();
}
