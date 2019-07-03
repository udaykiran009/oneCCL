#include <iccl.hpp>

#include "base.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <list>

using namespace std;

#define PRINT_BY_ROOT(fmt, ...)             \
    if(::rank == 0) {                       \
        printf(fmt"\n", ##__VA_ARGS__); }   \

void check_allreduce_on_comm(iccl_comm_t comm)
{
    size_t cur_comm_rank{};
    size_t cur_comm_size{};
    ICCL_CALL(iccl_get_comm_rank(comm, &cur_comm_rank));
    ICCL_CALL(iccl_get_comm_size(comm, &cur_comm_size));
    vector<float> send_buf(COUNT, cur_comm_rank);
    vector<float> recv_buf(COUNT, 0.0f);

    PRINT_BY_ROOT("Allreduce on %zu ranks", cur_comm_size);

    coll_attr.to_cache = 0;

    ICCL_CALL(iccl_allreduce(send_buf.data(),
                             recv_buf.data(),
                             COUNT,
                             iccl_dtype_float,
                             iccl_reduction_sum,
                             &coll_attr,
                             comm,
                             &request));
    ICCL_CALL(iccl_wait(request));

    float expected = (cur_comm_size - 1) * ((float) cur_comm_size / 2);

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

    iccl_comm_t comm;
    ICCL_CALL(iccl_comm_create(&comm, nullptr));

    check_allreduce_on_comm(comm);

    ICCL_CALL(iccl_comm_free(comm));
}

void check_max_comm_number()
{
    size_t user_comms = 0;

    PRINT_BY_ROOT("Create max number of communicators");
    std::vector<iccl_comm_t> communicators;

    iccl_status_t status = iccl_status_success;

    do
    {
        iccl_comm_t new_comm;
        status = iccl_comm_create(&new_comm, nullptr);

        ++user_comms;
        if (status != iccl_status_success)
        {
            break;
        }

        communicators.push_back(new_comm);

    } while(status == iccl_status_success);

    PRINT_BY_ROOT("created %zu communicators\n", user_comms);

    PRINT_BY_ROOT("Try to create one more communicator, it should fail");
    iccl_comm_t comm;
    status = iccl_comm_create(&comm, nullptr);
    if (status == iccl_status_success)
    {
        throw runtime_error("Extra communicator has been created");
    }

    PRINT_BY_ROOT("Free one comm, try to create again");
    size_t comm_idx = user_comms / 2;
    status = iccl_comm_free(communicators[comm_idx]);
    if (status != iccl_status_success)
    {
        throw runtime_error("Can't to free communicator");
    }

    status = iccl_comm_create(&communicators[comm_idx], nullptr);
    if (status != iccl_status_success)
    {
        throw runtime_error("Can't create communicator after free");
    }

    for(auto& comm_elem: communicators)
    {
        ICCL_CALL(iccl_comm_free(comm_elem));
    }
}

void check_comm_create_colored()
{
    PRINT_BY_ROOT("Create comm with color, ranks_count should be a power of 2 for test purpose");

    for (size_t split_by = 2; split_by <= size; split_by *= 2)
    {
        iccl_comm_t comm;
        iccl_comm_attr_t comm_attr;
        comm_attr.color = ::rank % split_by;
        size_t comm_size{};
        size_t comm_rank{};

        PRINT_BY_ROOT("Splitting global comm into %zu parts", split_by);
        ICCL_CALL(iccl_comm_create(&comm, &comm_attr));

        ICCL_CALL(iccl_get_comm_size(comm, &comm_size));
        ICCL_CALL(iccl_get_comm_rank(comm, &comm_rank));

        size_t expected_ranks_count = size / split_by;
        if (comm_size != expected_ranks_count)
        {
            throw runtime_error("Mismatch in size, expected " +
                                to_string(expected_ranks_count) +
                                " received " + to_string(comm_size));
        }

        PRINT_BY_ROOT("Global comm: idx=%zu, count=%zu; new comm: rank=%zu, size=%zu", ::rank,
                      size, comm_rank, comm_size);

        check_allreduce_on_comm(comm);

        ICCL_CALL(iccl_comm_free(comm));
    }
}

void check_comm_create_identical_color()
{
    iccl_comm_t comm;
    iccl_comm_attr_t comm_attr;
    comm_attr.color = 123;
    size_t comm_size{};
    size_t comm_rank{};

    PRINT_BY_ROOT("Create comm as a copy of the global one by settings identical colors");

    ICCL_CALL(iccl_comm_create(&comm, &comm_attr));
    ICCL_CALL(iccl_get_comm_size(comm, &comm_size));
    ICCL_CALL(iccl_get_comm_rank(comm, &comm_rank));

    if (comm_size != size)
    {
        throw runtime_error("Mismatch in size, expected " +
                            to_string(size) +
                            " received " + to_string(comm_size));
    }

    if (comm_rank != ::rank)
    {
        throw runtime_error("Mismatch in rank, expected " +
                            to_string(::rank) +
                            " received " + to_string(comm_rank));
    }

    PRINT_BY_ROOT("Global comm: rank=%zu, size=%zu; new comm: rank=%zu, size=%zu", ::rank,
                  size, comm_rank, comm_size);

    check_allreduce_on_comm(comm);

    ICCL_CALL(iccl_comm_free(comm));
}

int main()
{
    test_init();

    PRINT_BY_ROOT("Running communicators on %zu ranks", size);

    check_allreduce();

    check_max_comm_number();

    check_comm_create_colored();

    check_comm_create_identical_color();

    test_finalize();

    return 0;
}
