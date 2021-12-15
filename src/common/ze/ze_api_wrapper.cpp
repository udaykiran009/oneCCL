
#include "common/ze/ze_api_wrapper.hpp"

#include "common/global/global.hpp"
#include "common/stream/stream.hpp"
#include "common/log/log.hpp"
#include "sched/entry/ze/ze_primitives.hpp"

namespace ccl {

static void* libze_handle;
libze_ops_t libze_ops;

void ze_api_init() {
    std::string dll_name = ccl::global_data::env().ze_lib_path;
    if (dll_name.empty())
        dll_name = "libze_loader.so";

    libze_handle = dlopen(dll_name.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    CCL_THROW_IF_NOT(libze_handle, "could not dlopen: ", dll_name.c_str());

    void** ops = (void**)((void*)&libze_ops);
    int sym_count = sizeof(fn_names) / sizeof(*fn_names);

    for (int i = 0; i < sym_count; ++i) {
        ops[i] = dlsym(libze_handle, fn_names[i]);
        CCL_THROW_IF_NOT(ops[i], "dlsym is failed on: ", fn_names[i]);
        LOG_DEBUG("dlsym loaded of ", sym_count, " - ", i + 1, ": ", fn_names[i]);
    }
}

} //namespace ccl
