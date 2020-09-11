#include "kvs.hpp"

ccl::vector_class<char> stub_kvs::get(const ccl::string_class& prefix,
                                      const ccl::string_class& key) const {
    return {};
}

void stub_kvs::set(const ccl::string_class& prefix,
                   const ccl::string_class& key,
                   const ccl::vector_class<char>& data) const {}
