#pragma once
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/types_policy.hpp"
#include "oneapi/ccl/coll_attr_ids.hpp"
#include "oneapi/ccl/coll_attr_ids_traits.hpp"
#include "coll/coll_common_attributes.hpp"
namespace ccl {

class ccl_allgatherv_attr_impl_t : public ccl_operation_attr_impl_t {
public:
    using base_t = ccl_operation_attr_impl_t;

    ccl_allgatherv_attr_impl_t(
        const typename detail::ccl_api_type_attr_traits<operation_attr_id,
                                                        operation_attr_id::version>::type& version);
    ccl_allgatherv_attr_impl_t(const ccl_allgatherv_attr_impl_t& src);

private:
};
} // namespace ccl
