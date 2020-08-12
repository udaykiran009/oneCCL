#include "ccl_types.hpp"
#include "ccl_aliases.hpp"

#include "ccl_type_traits.hpp"
#include "ccl_types_policy.hpp"

#include "ccl_kvs.hpp"


class stub_kvs : public ccl::kvs_interface {
public:
    ccl::vector_class<char> get(const ccl::string_class& prefix, const ccl::string_class& key) const override;

    void set(const ccl::string_class& prefix,
                     const ccl::string_class& key,
                     const ccl::vector_class<char>& data) const override;

    ~stub_kvs() = default;
};
