#pragma once
#include <memory>

#include "ccl_types.hpp"
#include "ccl_types_policy.hpp"
#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"
#include "ccl_coll_attr.hpp"

#include "ccl_request.hpp"

class ccl_comm;
namespace ccl
{
class host_communicator
{
public:
    host_communicator(std::shared_ptr<ccl_comm> impl);

    size_t rank() const;
    size_t size() const;

    void barrier_impl();

    ccl::request_t
    barrier_impl(const barrier_attr_t& attr = default_barrier_attr_t);
    std::shared_ptr<ccl_comm> comm_impl;
};
}
