#ifndef ALLTOALLV_STRATEGY_HPP
#define ALLTOALLV_STRATEGY_HPP

struct alltoallv_strategy_impl
{
    size_t comm_size = 0;
    size_t* send_counts = nullptr;
    size_t* recv_counts = nullptr;

    alltoallv_strategy_impl(size_t size) : comm_size(size)
    {
        int result = posix_memalign((void**)&send_counts, ALIGNMENT, comm_size * sizeof(size_t));
        result = posix_memalign((void**)&recv_counts, ALIGNMENT, comm_size * sizeof(size_t));
        (void)result;
    }

    alltoallv_strategy_impl(const alltoallv_strategy_impl&) = delete;
    alltoallv_strategy_impl& operator=(const alltoallv_strategy_impl&) = delete;

    ~alltoallv_strategy_impl()
    {
        free(send_counts);
        free(recv_counts);
    }

    static constexpr const char* class_name() { return "alltoallv"; }

    template<class Dtype>
    void start_internal(ccl::communicator& comm, size_t count, const Dtype send_buf, Dtype recv_buf,
                        const ccl_coll_attr_t& coll_attr, ccl::stream_t& stream,
                        req_list_t& reqs)
    {
        for (size_t idx = 0; idx < comm_size; idx++)
        {
            send_counts[idx] = count;
            recv_counts[idx] = count;
        }
        reqs.push_back(comm.alltoallv(send_buf, send_counts,
                                      recv_buf, recv_counts,
                                      &coll_attr, stream));
    }
};

#endif /* ALLTOALLV_STRATEGY_HPP */
