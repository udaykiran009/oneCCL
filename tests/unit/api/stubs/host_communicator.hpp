#pragma once
#include <memory>

#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_coll_attr_ids.hpp"
#include "oneapi/ccl/ccl_coll_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_coll_attr.hpp"

#include "oneapi/ccl/ccl_request.hpp"

class ccl_comm;
namespace ccl {
class host_communicator {
public:
    host_communicator(std::shared_ptr<ccl_comm> impl);

    size_t rank() const;
    size_t size() const;

    void barrier_impl();

    ccl::request_t barrier_impl(const barrier_attr& attr = default_barrier_attr);
    std::shared_ptr<ccl_comm> comm_impl;
};
} // namespace ccl
