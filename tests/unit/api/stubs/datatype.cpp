#include "common/datatype/datatype.hpp"

ccl_datatype_storage::ccl_datatype_storage() {}
ccl_datatype_storage::~ccl_datatype_storage() {}

ccl::datatype ccl_datatype_storage::create(const ccl::datatype_attr& attr) {
    (void)attr;
    return {};
}

void ccl_datatype_storage::free(ccl::datatype idx) {
    (void)idx;
}

const ccl_datatype& ccl_datatype_storage::get(ccl::datatype idx) const {
    (void)idx;
    static ccl_datatype empty;
    return empty;
}

ccl::datatype& operator++(ccl::datatype& d) {
    using IntType = typename std::underlying_type<ccl::datatype>::type;
    d = static_cast<ccl::datatype>(static_cast<IntType>(d) + 1);
    return d;
}

ccl::datatype operator++(ccl::datatype& d, int) {
    ccl::datatype tmp(d);
    ++d;
    return tmp;
}
