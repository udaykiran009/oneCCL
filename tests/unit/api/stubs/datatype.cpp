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
