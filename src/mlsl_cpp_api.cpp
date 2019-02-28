#include "mlsl.hpp"
#include "common/global/global.hpp"

#include <utility>
#include <iostream>

#define CHECK_AND_THROW(result, diagnostic)     \
do {                                            \
    if(result != mlsl_status_success)           \
    {                                           \
        throw mlsl_error(diagnostic);           \
    }                                           \
} while (0);

namespace mlsl
{
class request_impl final : public request
{
public:
    explicit request_impl(mlsl_request_t mlsl_req) : req_detail(mlsl_req)
    {
        if(!req_detail)
        {
            //if the user calls synchronous collective op via mlsl_coll_attr_t->synchronous=1 then it will be progressed
            //in place and API will return null mlsl_request_t. In this case we mark cpp wrapper as completed,
            //all calls to wait() or test() will do nothing
            completed = true;
        }
    }

    ~request_impl()
    {
        if (!completed)
        {
            //todo: rework MLSL_LOG(ERROR, ...) to not throw/terminate and use it here
            std::cerr << "Not completed request is destroyed" << std::endl;
        }
    }

    void wait() final
    {
        if (!completed)
        {
            mlsl_status_t result = mlsl_wait(req_detail);
            CHECK_AND_THROW(result, "Wait for request completion has been failed");
            completed = true;
        }
    }

    bool test() final
    {
        if (!completed)
        {
            int is_completed = 0;
            mlsl_test(req_detail, &is_completed);
            completed = is_completed != 0;
        }
        return completed;
    }

private:
    mlsl_request_t req_detail;
    bool completed = false;
};
}

using namespace mlsl;

bool MLSL_API environment::is_initialized = false;

MLSL_API environment::environment()
{
    if (!is_initialized)
    {
        mlsl_status_t result = mlsl_init();
        CHECK_AND_THROW(result, "Failed to initialize mlsl");
        is_initialized = true;
    }
    else
    {
        throw mlsl::mlsl_error("Already initialized");
    }
}

MLSL_API environment::~environment()
{
    if (is_initialized)
    {
        mlsl_finalize();
        is_initialized = false;
    }
}

MLSL_API communicator::communicator()
{
    //default constructor uses global communicator
    //todo: possibly we should introduce some proxy/getter for global data class to make unit testing easier
    comm_impl = global_data.comm;
}

MLSL_API communicator::communicator(mlsl_comm_attr_t* comm_attr)
{
    if(!comm_attr)
    {
        comm_impl = global_data.comm;
    }
    else
    {
        comm_impl = std::shared_ptr<mlsl_comm>(mlsl_comm::create_with_color(comm_attr->color, global_data.comm_ids.get(), global_data.comm.get()));
    }
}

MLSL_API size_t communicator::rank()
{
    return comm_impl->rank();
}

MLSL_API size_t communicator::size()
{
    return comm_impl->size();
}

std::pair<size_t, size_t> MLSL_API communicator::priority_range()
{
    size_t min_priority = 0;
    size_t max_priority = 0;
    mlsl_get_priority_range(&min_priority, &max_priority);

    return std::make_pair(min_priority, max_priority);
}

std::shared_ptr<mlsl::request> MLSL_API communicator::bcast(void* buf,
                                                            size_t count,
                                                            mlsl::data_type dtype,
                                                            size_t root,
                                                            const mlsl_coll_attr_t* attributes)
{
    mlsl_request_t mlsl_req;
    auto mlsl_data_type = static_cast<mlsl_datatype_t>(dtype);

    mlsl_status_t result = mlsl_bcast(buf, count, mlsl_data_type, root, attributes, comm_impl.get(), &mlsl_req);
    CHECK_AND_THROW(result, "Broadcast failed");

    return std::make_shared<mlsl::request_impl>(mlsl_req);
}

std::shared_ptr<mlsl::request> MLSL_API communicator::reduce(const void* send_buf,
                                                             void* recv_buf,
                                                             size_t count,
                                                             mlsl::data_type dtype,
                                                             mlsl::reduction reduction,
                                                             size_t root,
                                                             const mlsl_coll_attr_t* attributes)
{
    mlsl_request_t mlsl_req;
    auto mlsl_data_type = static_cast<mlsl_datatype_t>(dtype);
    auto mlsl_reduction_type = static_cast<mlsl_reduction_t>(reduction);

    mlsl_status_t result = mlsl_reduce(send_buf, recv_buf, count, mlsl_data_type, mlsl_reduction_type,
                                       root, attributes, comm_impl.get(), &mlsl_req);
    CHECK_AND_THROW(result, "Reduce failed");

    return std::make_shared<mlsl::request_impl>(mlsl_req);
}

std::shared_ptr<mlsl::request> MLSL_API communicator::allreduce(const void* send_buf,
                                                                void* recv_buf,
                                                                size_t count,
                                                                mlsl::data_type dtype,
                                                                mlsl::reduction reduction,
                                                                const mlsl_coll_attr_t* attributes)
{
    mlsl_request_t mlsl_req;
    auto mlsl_data_type = static_cast<mlsl_datatype_t>(dtype);
    auto mlsl_reduction_type = static_cast<mlsl_reduction_t>(reduction);

    mlsl_status_t result = mlsl_allreduce(send_buf, recv_buf, count, mlsl_data_type, mlsl_reduction_type,
                                          attributes, comm_impl.get(), &mlsl_req);
    CHECK_AND_THROW(result, "Allreduce failed");

    return std::make_shared<mlsl::request_impl>(mlsl_req);
}

std::shared_ptr<mlsl::request> MLSL_API communicator::allgatherv(const void* send_buf,
                                                                 size_t send_count,
                                                                 void* recv_buf,
                                                                 size_t* recv_counts,
                                                                 mlsl::data_type dtype,
                                                                 const mlsl_coll_attr_t* attributes)
{
    mlsl_request_t mlsl_req;
    auto mlsl_data_type = static_cast<mlsl_datatype_t>(dtype);

    mlsl_status_t result = mlsl_allgatherv(send_buf, send_count, recv_buf, recv_counts, mlsl_data_type, attributes,
                                           comm_impl.get(), &mlsl_req);
    CHECK_AND_THROW(result, "Allgatherv failed");

    return std::make_shared<mlsl::request_impl>(mlsl_req);
}

void MLSL_API communicator::barrier()
{
    mlsl_barrier(comm_impl.get());
}

std::shared_ptr<mlsl::request> MLSL_API communicator::sparse_allreduce(const void* send_ind_buf,
                                                                size_t send_ind_count,
                                                                const void* send_val_buf,
                                                                size_t send_val_count,
                                                                void** recv_ind_buf,
                                                                size_t* recv_ind_count,
                                                                void** recv_val_buf,
                                                                size_t* recv_val_count,
                                                                mlsl::data_type index_dtype,
                                                                mlsl::data_type value_dtype,
                                                                mlsl::reduction reduction,
                                                                const mlsl_coll_attr_t* attributes)
{
    mlsl_request_t mlsl_req;
    auto mlsl_index_type = static_cast<mlsl_datatype_t>(index_dtype);
    auto mlsl_data_type = static_cast<mlsl_datatype_t>(value_dtype);
    auto mlsl_reduction_type = static_cast<mlsl_reduction_t>(reduction);

    mlsl_status_t result = mlsl_sparse_allreduce(send_ind_buf, send_ind_count, send_val_buf, send_val_count, recv_ind_buf,
                                                 recv_ind_count, recv_val_buf, recv_val_count, mlsl_index_type, mlsl_data_type,
                                                 mlsl_reduction_type, attributes, comm_impl.get(), &mlsl_req);
    CHECK_AND_THROW(result, "Sparse allreduce failed");

    return std::make_shared<mlsl::request_impl>(mlsl_req);
}