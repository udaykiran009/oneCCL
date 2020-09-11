#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_kvs.hpp"

namespace ccl {

class kvs_impl {
public:
    //STUB

    kvs::address_type addr;
};

kvs::address_type CCL_API kvs::get_address() const {
    //TODO: add logic;
    //static kvs_impl tmp;
    //return tmp.addr;
    static array_class<char, 256> tmp;
    return tmp;
}

vector_class<char> CCL_API kvs::get(const string_class& prefix, const string_class& key) const {
    //TODO: add logic;
    throw;
}

void CCL_API kvs::set(const string_class& prefix,
                      const string_class& key,
                      const vector_class<char>& data) const {
    //TODO: add logic;
    throw;
}
CCL_API kvs::~kvs() {
    //TODO: add logic;
}

CCL_API kvs::kvs(const kvs::address_type& addr) {
    //TODO: add logic;
}

CCL_API kvs::kvs() {
    //TODO: add logic;
}

} // namespace ccl
