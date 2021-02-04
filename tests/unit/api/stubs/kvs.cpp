#include "kvs.hpp"
#include "kvs_impl.hpp"

ccl::vector_class<char> stub_kvs::get(const ccl::string_class& key) {
    return {};
}

void stub_kvs::set(const ccl::string_class& key, const ccl::vector_class<char>& data) {}

namespace ccl {

kvs_impl::kvs_impl(const kvs_attr& attr) {}

kvs_impl::kvs_impl(const kvs::address_type& addr, const kvs_attr& attr) {
    (void)addr;
}

kvs::address_type kvs_impl::get_addr() {
    //    static kvs::address_type empty_addr;
    return addr;
}

vector_class<char> kvs_impl::get(const string_class& key) {
    static vector_class<char> empty_vec;
    return empty_vec;
}

void kvs_impl::set(const string_class& key, const vector_class<char>& data) {
    (void)key;
    (void)data;
}

std::shared_ptr<internal_kvs> kvs_impl::get() const {
    std::shared_ptr<internal_kvs> empty_kvs;
    return empty_kvs;
}
} // namespace ccl

ccl::kvs::~kvs() {}

ccl::kvs::address_type ccl::kvs::get_address() const {
    ccl::kvs::address_type empty;
    return empty;
}

ccl::vector_class<char> ccl::kvs::get(const ccl::string_class& key) {
    (void)key;
    ccl::vector_class<char> empty;
    return empty;
}

void ccl::kvs::set(const ccl::string_class& key, const ccl::vector_class<char>& data) {
    (void)key;
    (void)data;
}

ccl::kvs::kvs(const kvs_attr& attr) {}

ccl::kvs::kvs(const ccl::kvs::address_type& addr, const kvs_attr& attr) {
    (void)addr;
}
const ccl::kvs_impl& ccl::kvs::get_impl() {
    return *pimpl;
}
