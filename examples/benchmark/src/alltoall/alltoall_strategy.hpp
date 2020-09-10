#ifndef ALLTOALL_STRATEGY_HPP
#define ALLTOALL_STRATEGY_HPP

struct alltoall_strategy_impl
{
    static constexpr const char* class_name() { return "alltoall"; }

    template<class Dtype, class comm_t, class ...Args>
    void start_internal(comm_t& comm, size_t count, const Dtype send_buf, Dtype recv_buf,
                        const bench_coll_exec_attr& bench_attr,
                        req_list_t& reqs, Args&&...args)
    {
        reqs.push_back(comm.alltoall(send_buf, recv_buf, count, bench_attr.get_attr<ccl::alltoall_attr>(), std::forward<Args>(args)...));
    }
};

#endif /* ALLTOALL_STRATEGY_HPP */
