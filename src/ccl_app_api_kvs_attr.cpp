#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_kvs_attr_ids.hpp"
#include "oneapi/ccl/ccl_kvs_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_kvs_attr.hpp"

// Core file with PIMPL implementation
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable/kvs/kvs_common_attr.hpp"
#include "kvs_attr_impl.hpp"

namespace ccl {
#define kvsA ,
#define API_FORCE_INSTANTIATION(class_name, IN_attrId, IN_Value, OUT_Traits_Value) \
    template CCL_API IN_Value class_name::set<IN_attrId, IN_Value>(const IN_Value& v); \
\
    template CCL_API const typename detail::OUT_Traits_Value<kvs_attr_id, \
                                                              IN_attrId>::type& \
    class_name::get<IN_attrId>() const; \
\
    template CCL_API bool class_name::is_valid<IN_attrId>() const noexcept;

/**
 * kvs_attr attributes definition
 */
CCL_API kvs_attr::kvs_attr(ccl_empty_attr)
    : base_t(std::shared_ptr<impl_t>(new impl_t(ccl_empty_attr::version))) {}
CCL_API kvs_attr::kvs_attr(kvs_attr&& src)
    : base_t(std::move(src)) {}

CCL_API kvs_attr::kvs_attr(const kvs_attr& src)
    : base_t(src) {}

CCL_API kvs_attr::kvs_attr(
    const typename detail::ccl_api_type_attr_traits<kvs_attr_id,
        kvs_attr_id::version>::return_type& version)
    : base_t(std::shared_ptr<impl_t>(new impl_t(version))) {}

CCL_API kvs_attr::~kvs_attr() noexcept {}

CCL_API kvs_attr& kvs_attr::operator=(const kvs_attr& src) {
    this->get_impl() = src.get_impl();
    return *this;
}

CCL_API kvs_attr& kvs_attr::operator=(kvs_attr&& src) {
    if (src.get_impl() != this->get_impl()) {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}
API_FORCE_INSTANTIATION(kvs_attr,
                        kvs_attr_id::version,
                        ccl::library_version,
                        ccl_api_type_attr_traits)

#undef API_FORCE_INSTANTIATION
#undef kvsA
} // namespace ccl
