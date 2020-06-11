#ifndef ALLTOALL_STRATEGY_HPP
#define ALLTOALL_STRATEGY_HPP

struct alltoall_strategy_impl
{
    static constexpr const char* class_name() { return "alltoall"; }

    template<class Dtype>
    void start_internal(ccl::communicator &comm, size_t count, const Dtype send_buf, Dtype recv_buf,
                        const bench_coll_exec_attr& bench_attr, ccl::stream_t& stream,
                        req_list_t& reqs)
    {
        reqs.push_back(comm.alltoall(send_buf, recv_buf, count, &bench_attr.coll_attr, stream));
    }
};

#endif /* ALLTOALL_STRATEGY_HPP */
