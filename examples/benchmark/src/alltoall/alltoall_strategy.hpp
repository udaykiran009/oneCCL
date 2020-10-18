#pragma once

struct alltoall_strategy_impl {
    static constexpr const char* class_name() {
        return "alltoall";
    }

    static const ccl::alltoall_attr& get_op_attr(const bench_exec_attr& bench_attr) {
        return bench_attr.get_attr<ccl::alltoall_attr>();
    }

    template <class Dtype, class... Args>
    void start_internal(ccl::communicator& comm,
                        size_t count,
                        const Dtype send_buf,
                        Dtype recv_buf,
                        const bench_exec_attr& bench_attr,
                        req_list_t& reqs,
                        Args&&... args) {
        reqs.push_back(ccl::alltoall(send_buf, recv_buf, count, comm, std::forward<Args>(args)...));
    }
};
