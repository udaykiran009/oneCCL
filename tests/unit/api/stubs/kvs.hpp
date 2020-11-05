#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/aliases.hpp"

#include "oneapi/ccl/type_traits.hpp"
#include "oneapi/ccl/types_policy.hpp"

#include "oneapi/ccl/kvs_attr_ids.hpp"
#include "oneapi/ccl/kvs_attr_ids_traits.hpp"
#include "oneapi/ccl/kvs_attr.hpp"

#include "oneapi/ccl/kvs.hpp"

class stub_kvs : public ccl::kvs_interface {
public:
    ccl::vector_class<char> get(const ccl::string_class& key) override;

    void set(const ccl::string_class& key, const ccl::vector_class<char>& data) override;

    ~stub_kvs() = default;
};
