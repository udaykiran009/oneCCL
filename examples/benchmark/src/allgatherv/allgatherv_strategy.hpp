#ifndef ALLGATHERV_STRATEGY_HPP
#define ALLGATHERV_STRATEGY_HPP

struct allgatherv_strategy_impl
{
    size_t comm_size = 0;
    size_t* recv_counts = nullptr;
    allgatherv_strategy_impl(size_t size) : comm_size(size)
    {
        int result = posix_memalign((void**)&recv_counts, ALIGNMENT, comm_size * sizeof(size_t));
        (void)result;
    }

    allgatherv_strategy_impl(const allgatherv_strategy_impl&) = delete;
    allgatherv_strategy_impl& operator=(const allgatherv_strategy_impl&) = delete;

    ~allgatherv_strategy_impl()
    {
        free(recv_counts);
    }

    static constexpr const char* class_name() { return "allgatherv"; }

    template<class Dtype>
    void start_internal(ccl::communicator& comm, size_t count, const Dtype send_buf, Dtype recv_buf,
                        const ccl::coll_attr& attr, ccl::stream_t& stream,
                        req_list_t& reqs)
    {
        for (size_t idx = 0; idx < comm_size; idx++)
        {
            recv_counts[idx] = count;
        }
        reqs.push_back(comm.allgatherv(send_buf, count,
                                       recv_buf, recv_counts,
                                       &attr, stream));
    }
};

#endif /* ALLGATHER_STRATEGY_HPP */
