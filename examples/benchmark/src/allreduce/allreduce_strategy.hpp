#ifndef ALLREDUCE_STRATEGY_HPP
#define ALLREDUCE_STRATEGY_HPP

struct allreduce_strategy_impl
{
    static constexpr const char* class_name() { return "allreduce"; }

    template<class Dtype>
    void start_internal(ccl::communicator &comm, size_t count, const Dtype send_buf, Dtype recv_buf,
                        const ccl_coll_attr_t& coll_attr, ccl::stream_t& stream,
                        req_list_t& reqs)
    {
        reqs.push_back(comm.allreduce(send_buf, recv_buf, count, ccl::reduction::sum,
                                      &coll_attr, stream));
    }
};

#endif /* ALLREDUCE_STRATEGY_HPP */
