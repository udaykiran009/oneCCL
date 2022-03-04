#include "common/api_wrapper/api_wrapper.hpp"
#include "common/api_wrapper/ofi_api_wrapper.hpp"

namespace ccl {

lib_info_t ofi_lib_info;
ofi_lib_ops_t ofi_lib_ops;

bool ofi_api_init() {
    bool ret = true;

    ofi_lib_info.ops = &ofi_lib_ops;
    ofi_lib_info.fn_names = ofi_fn_names;

    // lib_path specifies the name and full path to the OFI library
    // it should be absolute and validated path
    // pointing to desired libfabric library
    ofi_lib_info.path = ccl::global_data::env().ofi_lib_path;

    if (ofi_lib_info.path.empty()) {
        ofi_lib_info.path = "libfabric.so.1";
    }
    LOG_DEBUG("OFI lib path: ", ofi_lib_info.path);

    load_library(ofi_lib_info);
    if (!ofi_lib_info.handle)
        ret = false;

    return ret;
}

void ofi_api_fini() {
    LOG_DEBUG("close OFI lib: handle:", ofi_lib_info.handle);
    close_library(ofi_lib_info);
}

} //namespace ccl
