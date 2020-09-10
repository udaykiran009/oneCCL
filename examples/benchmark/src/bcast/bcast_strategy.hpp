#ifndef BCAST_STRATEGY_HPP
#define BCAST_STRATEGY_HPP

struct bcast_strategy_impl
{
    static constexpr const char* class_name() { return "bcast"; }

    template<class Dtype, class comm_t, class ...Args>
    void start_internal(comm_t& comm, size_t count, Dtype send_buf, Dtype recv_buf,
                        const bench_coll_exec_attr& bench_attr,
                        req_list_t& reqs, Args&&...args)
    {
        (void)send_buf;
        reqs.push_back(comm.broadcast(recv_buf, count, COLL_ROOT, bench_attr.get_attr<ccl::broadcast_attr>(), std::forward<Args>(args)...));
    }
};

#endif /* BCAST_STRATEGY_HPP */
