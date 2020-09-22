#include "kvs.hpp"

ccl::string_class stub_kvs::get(const ccl::string_class& prefix,
                                const ccl::string_class& key) const {
    return {};
}

void stub_kvs::set(const ccl::string_class& prefix,
                   const ccl::string_class& key,
                   const ccl::string_class& data) const {}
