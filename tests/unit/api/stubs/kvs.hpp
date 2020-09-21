#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"

#include "oneapi/ccl/ccl_kvs.hpp"

class stub_kvs : public ccl::kvs_interface {
public:
    ccl::vector_class<char> get(const ccl::string_class& key) const override;

    void set(const ccl::string_class& key,
             const ccl::vector_class<char>& data) const override;

    ~stub_kvs() = default;
};
