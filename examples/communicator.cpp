#include "base.hpp"

#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

#define PRINT_BY_ROOT(fmt, ...)             \
    if(::rank == 0) {                       \
        printf(fmt"\n", ##__VA_ARGS__); }   \

void check_allreduce_on_comm(mlsl_comm_t comm)
{
    size_t cur_comm_rank_idx = mlsl_get_comm_rank(comm);
    size_t cur_comm_ranks_count = mlsl_get_comm_size(comm);
    vector<float> send_buf(COUNT, cur_comm_rank_idx);
    vector<float> recv_buf(COUNT, 0.0f);

    PRINT_BY_ROOT("Allreduce on %zu ranks", cur_comm_ranks_count);

    coll_attr.to_cache = 0;

    MLSL_CALL(mlsl_allreduce(send_buf.data(),
                             recv_buf.data(),
                             COUNT,
                             mlsl_dtype_float,
                             mlsl_reduction_sum,
                             &coll_attr,
                             comm,
                             &request));
    MLSL_CALL(mlsl_wait(request));

    float expected = (cur_comm_ranks_count - 1) * ((float) cur_comm_ranks_count / 2);

    for(size_t i = 0; i < recv_buf.size(); ++i)
    {
        if (recv_buf[i] != expected)
        {
            throw runtime_error("Recv[ " + to_string(i) + "]= " + to_string(recv_buf[i]) +
                    ", expected " + to_string(expected));
        }
    }
}

void check_allreduce()
{
    PRINT_BY_ROOT("Create new communicator as a copy of the global one and check that allreduce works");

    mlsl_comm_t comm;
    MLSL_CALL(mlsl_comm_create(&comm, nullptr));

    check_allreduce_on_comm(comm);

    MLSL_CALL(mlsl_comm_free(comm));
}

void check_max_comm_number()
{
    //skip world communicator
    const size_t max_comms = 65534;
    PRINT_BY_ROOT("Create max number of communicators");
    vector<mlsl_comm_t> communicators(max_comms);

    for (size_t i = 0; i < max_comms; ++i)
    {
        mlsl_status_t status = mlsl_comm_create(&communicators[i], nullptr);
        if (status != mlsl_status_success)
        {
            throw runtime_error("Can't create communicator " + to_string(i));
        }
    }

    PRINT_BY_ROOT("Try to create one more communicator, it should fail");
    mlsl_comm_t comm;
    mlsl_status_t status = mlsl_comm_create(&comm, nullptr);
    if (status == mlsl_status_success)
    {
        throw runtime_error("Extra communicator has been created");
    }

    PRINT_BY_ROOT("Free one comm, try to create again");
    size_t comm_idx = max_comms / 2;
    status = mlsl_comm_free(communicators[comm_idx]);
    if (status != mlsl_status_success)
    {
        throw runtime_error("Can't to free communicator");
    }

    status = mlsl_comm_create(&communicators[comm_idx], nullptr);
    if (status != mlsl_status_success)
    {
        throw runtime_error("Can't create communicator after free");
    }

    for(auto& comm: communicators)
    {
        MLSL_CALL(mlsl_comm_free(comm));
    }
}

void check_comm_create_colored()
{
    PRINT_BY_ROOT("Create comm with color, ranks_count should be a power of 2 for test purpose");

    for (size_t split_by = 2; split_by <= size; split_by *= 2)
    {
        mlsl_comm_t comm;
        mlsl_comm_attr_t comm_attr;
        comm_attr.color = ::rank % split_by;

        PRINT_BY_ROOT("Splitting global comm into %zu parts", split_by);
        MLSL_CALL(mlsl_comm_create(&comm, &comm_attr));

        size_t expected_ranks_count = mlsl_get_comm_size(nullptr) / split_by;
        if (mlsl_get_comm_size(comm) != expected_ranks_count)
        {
            throw runtime_error("Mismatch in size, expected " +
                                to_string(expected_ranks_count) +
                                " received " + to_string(mlsl_get_comm_size(comm)));
        }

        PRINT_BY_ROOT("Global comm: idx=%zu, count=%zu; new comm: idx=%zu, count=%zu", ::rank,
                      size, mlsl_get_comm_rank(comm), mlsl_get_comm_size(comm));

        check_allreduce_on_comm(comm);

        MLSL_CALL(mlsl_comm_free(comm));
    }
}

void check_comm_create_identical_color()
{
    mlsl_comm_t comm;
    mlsl_comm_attr_t comm_attr;
    comm_attr.color = 123;

    PRINT_BY_ROOT("Create comm as a copy of the global one by settings identical colors");

    MLSL_CALL(mlsl_comm_create(&comm, &comm_attr));

    if (mlsl_get_comm_size(comm) != size)
    {
        throw runtime_error("Mismatch in size, expected " +
                            to_string(size) +
                            " received " + to_string(mlsl_get_comm_size(comm)));
    }

    if (mlsl_get_comm_rank(comm) != ::rank)
    {
        throw runtime_error("Mismatch in rank, expected " +
                            to_string(::rank) +
                            " received " + to_string(mlsl_get_comm_rank(comm)));
    }

    PRINT_BY_ROOT("Global comm: idx=%zu, count=%zu; new comm: idx=%zu, count=%zu", ::rank,
                  size, mlsl_get_comm_rank(comm), mlsl_get_comm_size(comm));

    check_allreduce_on_comm(comm);

    MLSL_CALL(mlsl_comm_free(comm));
}

int main()
{
    MLSL_CALL(mlsl_init());

    ::rank = mlsl_get_comm_rank(nullptr);
    size = mlsl_get_comm_size(nullptr);

    PRINT_BY_ROOT("Running communicators on %zu ranks", size);

    check_allreduce();

    check_max_comm_number();

    check_comm_create_colored();

    check_comm_create_identical_color();

    MLSL_CALL(mlsl_finalize());

    return 0;
}
