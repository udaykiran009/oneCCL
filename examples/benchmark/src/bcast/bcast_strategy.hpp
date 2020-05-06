#ifndef BCAST_STRATEGY_HPP
#define BCAST_STRATEGY_HPP

struct bcast_strategy_impl
{
    static constexpr const char* class_name() { return "bcast"; }

    template<class Dtype>
    void start_internal(ccl::communicator &comm, size_t count, Dtype send_buf, Dtype recv_buf,
                        const ccl_coll_attr_t& coll_attr, ccl::stream_t& stream,
                        req_list_t& reqs)
    {
        (void)send_buf;
        reqs.push_back(comm.bcast(recv_buf, count, COLL_ROOT, &coll_attr, stream));
    }
};

#endif /* BCAST_STRATEGY_HPP */
