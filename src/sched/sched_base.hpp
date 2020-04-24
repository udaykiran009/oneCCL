#pragma once
#include "atl/atl.h"
#include "coll/coll.hpp"
#include "sched/entry/entry.hpp"
#include "common/request/request.hpp"

#include <deque>
#include <list>
#include <memory>


class ccl_sched_queue;
class ccl_sched_bin;
class ccl_request;
class ccl_parallelizer;
class ccl_executor;

enum ccl_sched_internal_type
{
    ccl_sched_internal_none,
    ccl_sched_internal_fusion,
    ccl_sched_internal_unordered_coll
};

enum ccl_sched_add_mode
{
    ccl_sched_add_front,
    ccl_sched_add_back,

    ccl_sched_add_mode_last_value
};

std::string to_string(ccl_sched_add_mode mode);

struct ccl_sched_buffer_handler
{
    ccl_buffer buffer;
    size_t size;

    ccl_sched_buffer_handler(ccl_buffer buffer, size_t size)
        : buffer(buffer), size(size) {}
};

struct ccl_sched_memory
{
    std::list<ccl_sched_buffer_handler> buf_list;
    std::list<atl_mr_t*> mr_list;
};

static size_t lifo_priority = 0;

struct ccl_sched_base
{
    template<ccl_sched_add_mode mode = ccl_sched_add_mode_last_value>
    using add_entry_mode_t = std::integral_constant<ccl_sched_add_mode, mode>;

    using add_entry_front_t = add_entry_mode_t<ccl_sched_add_front>;
    using add_entry_back_t = add_entry_mode_t<ccl_sched_add_back>;

    void set_coll_attr(const ccl_coll_attr& attr);

    void update_coll_param_and_attr(const ccl_coll_param& param,
                                    const ccl_coll_attr& attr);

    size_t get_priority() const;
    ccl_buffer alloc_buffer(size_t bytes);
    void alloc_buffer_ptr(void**& out_ptr_to_allocated_ptr, size_t bytes);
    ccl_buffer update_buffer(ccl_buffer buffer, size_t new_size);
    ccl_buffer find_and_realloc_buffer(void* buffer, size_t new_size, size_t expected_size = 0);
    void free_buffers();

    void add_memory_region(atl_mr_t* mr);

    void alloc_buffers_for_sycl_copy();

    void set_entry_exec_mode(ccl_sched_entry_exec_mode mode)
    {
        exec_mode = mode;
    }

    void set_add_mode(ccl_sched_add_mode mode)
    {
        add_mode = mode;
    }

    ccl_coll_param coll_param{};

    ccl_coll_attr coll_attr{};

    ccl_coll_param_copy coll_param_copy{};

    /* sequence number of the schedule in the communicator */
    ccl_sched_id_t sched_id = 0;

    /* whether sched was created by internal module (fusion_manager/unordered_coll_manager) */
    ccl_sched_internal_type internal_type = ccl_sched_internal_none;

    static size_t get_lifo_priority() noexcept
    {
        return lifo_priority++;
    }

protected:

    ~ccl_sched_base() = default;

    ccl_sched_base(const ccl_coll_param& coll_param) :
        coll_param(coll_param)
    {}

    void update_id()
    {
        sched_id = coll_param.comm->get_sched_id(internal_type != ccl_sched_internal_none);
    }

    void dump(std::ostream& out, const char *name) const;

    ccl_sched_memory memory;
    ccl_sched_entry_exec_mode exec_mode = ccl_sched_entry_exec_regular;
    ccl_sched_add_mode add_mode = ccl_sched_add_back;
};
