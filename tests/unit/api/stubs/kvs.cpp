#include "kvs.hpp"

ccl::vector_class<char> stub_kvs::get(const ccl::string_class& key) const {
    return {};
}

void stub_kvs::set(const ccl::string_class& key, const ccl::vector_class<char>& data) const {}

namespace ccl {
class kvs_impl {};
} // namespace ccl

ccl::kvs::~kvs() {}

ccl::kvs::address_type ccl::kvs::get_address() const {
    ccl::kvs::address_type empty;
    return empty;
}

ccl::vector_class<char> ccl::kvs::get(const ccl::string_class& key) const {
    (void)key;
    ccl::vector_class<char> empty;
    return empty;
}

void ccl::kvs::set(const ccl::string_class& key, const ccl::vector_class<char>& data) const {
    (void)key;
    (void)data;
}

ccl::kvs::kvs(const kvs_attr& attr) {}

ccl::kvs::kvs(const ccl::kvs::address_type& addr, const kvs_attr& attr) {
    (void)addr;
}
const ccl::kvs_impl& ccl::kvs::get_impl() {
    static ccl::kvs_impl kvs_empty;
    return kvs_empty;
}
