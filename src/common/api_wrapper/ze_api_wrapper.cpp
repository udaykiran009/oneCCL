#include "common/api_wrapper/api_wrapper.hpp"
#include "common/api_wrapper/ze_api_wrapper.hpp"
#include "common/stream/stream.hpp"
#include "sched/entry/ze/ze_primitives.hpp"

namespace ccl {

ccl::lib_info_t ze_lib_info;
ze_lib_ops_t ze_lib_ops;

bool ze_api_init() {
    bool ret = true;

    ze_lib_info.ops = &ze_lib_ops;
    ze_lib_info.fn_names = ze_fn_names;

    // lib_path specifies the name and full path to the level-zero library
    // it should be absolute and validated path
    // pointing to desired libze_loader library
    ze_lib_info.path = ccl::global_data::env().ze_lib_path;

    if (ze_lib_info.path.empty()) {
        ze_lib_info.path = "libze_loader.so";
    }
    LOG_DEBUG("level-zero lib path: ", ze_lib_info.path);

    load_library(ze_lib_info);
    if (!ze_lib_info.handle)
        ret = false;

    return ret;
}

void ze_api_fini() {
    LOG_DEBUG("close level-zero lib: handle:", ze_lib_info.handle);
    close_library(ze_lib_info);
}

} //namespace ccl
