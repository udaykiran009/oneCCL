#ifndef COLL_HPP
#define COLL_HPP

#include "base.hpp"
#include "config.hpp"

struct base_coll;

using coll_list_t = std::vector<std::shared_ptr<base_coll>>;
using req_list_t = std::vector<std::unique_ptr<ccl::request>>;

/* base polymorph collective wrapper class */
struct base_coll
{
    virtual ~base_coll() = default;

    virtual const char* name() const noexcept { return nullptr; };

    virtual void prepare(size_t elem_count) {};
    virtual void finalize(size_t elem_count) {};

    virtual ccl::datatype get_dtype() const = 0;

    virtual void start(size_t count, size_t buf_idx,
                       const ccl::coll_attr& attr,
                       req_list_t& reqs) = 0;

    virtual void start_single(size_t count,
                              const ccl::coll_attr& attr,
                              req_list_t& reqs) = 0;

    void* send_bufs[BUF_COUNT] = { nullptr };
    void* recv_bufs[BUF_COUNT] = { nullptr };
    void* single_send_buf = nullptr;
    void* single_recv_buf = nullptr;

    /* global communicator & stream for all collectives */
    static ccl::communicator_t comm;
    static ccl::stream_t stream;
};

ccl::communicator_t base_coll::comm;
ccl::stream_t base_coll::stream;

#endif /* COLL_HPP */
