#include "common/api_wrapper/api_wrapper.hpp"
#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
#include "common/api_wrapper/ze_api_wrapper.hpp"
#endif //CCL_ENABLE_SYCL && CCL_ENABLE_ZE
#if defined(CCL_ENABLE_MPI)
#include "common/api_wrapper/mpi_api_wrapper.hpp"
#endif //CCL_ENABLE_MPI
#include "common/api_wrapper/ofi_api_wrapper.hpp"

#include <dlfcn.h>

namespace ccl {

void api_wrappers_init() {
    bool ofi_inited = true, mpi_inited = true;
    if (!(ofi_inited = ofi_api_init())) {
        LOG_INFO("could not initialize OFI api");
    }
#if defined(CCL_ENABLE_MPI)
    if (!(mpi_inited = mpi_api_init())) {
        LOG_INFO("could not initialize MPI api");
    }
#endif //CCL_ENABLE_MPI
    CCL_THROW_IF_NOT(ofi_inited || mpi_inited, "could not initialize any transport library");
    if (!ofi_inited && (ccl::global_data::env().atl_transport == ccl_atl_ofi)) {
        ccl::global_data::env().atl_transport = ccl_atl_mpi;
        LOG_WARN("OFI transport was not initialized, fallback to MPI transport");
    }

    if (!mpi_inited && (ccl::global_data::env().atl_transport == ccl_atl_mpi)) {
        ccl::global_data::env().atl_transport = ccl_atl_ofi;
        LOG_WARN("MPI transport was not initialized, fallback to OFI transport");
    }

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    if (ccl::global_data::env().backend == backend_mode::native &&
        ccl::global_data::env().ze_enable) {
        LOG_INFO("initializing level-zero api");
        if (ze_api_init()) {
            try {
                ccl::global_data::get().ze_data.reset(new ze::global_data_desc);
            }
            catch (const ccl::exception& e) {
                LOG_INFO("could not initialize level-zero: ", e.what());
            }
            catch (...) {
                LOG_INFO("could not initialize level-zero: unknown error");
            }
        }
    }
    else {
        LOG_INFO("could not initialize level-zero api");
    }
#endif //CCL_ENABLE_SYCL && CCL_ENABLE_ZE
}

void api_wrappers_fini() {
    ofi_api_fini();
#if defined(CCL_ENABLE_MPI)
    mpi_api_fini();
#endif //CCL_ENABLE_MPI
#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    ze_api_fini();
#endif //CCL_ENABLE_SYCL && CCL_ENABLE_ZE
}

void load_library(lib_info_t& info) {
    //TODO: MLSL-1384, finish with parse of lib_path
    info.handle = dlopen(info.path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    if (!info.handle) {
        LOG_WARN("could not open the library: ", info.path.c_str(), ", error: ", dlerror());
        return;
    }

    void** ops = (void**)((void*)info.ops);
    auto fn_names = info.fn_names;
    for (size_t i = 0; i < fn_names.size(); ++i) {
        ops[i] = dlsym(info.handle, fn_names[i].c_str());
        CCL_THROW_IF_NOT(ops[i], "dlsym is failed on: ", fn_names[i], ", error: ", dlerror());
        LOG_DEBUG("dlsym loaded of ", fn_names.size(), " - ", i + 1, ": ", fn_names[i]);
    }
}

void close_library(lib_info_t& info) {
    if (info.handle) {
        dlclose(info.handle);
        info.handle = nullptr;
    }
}

} //namespace ccl
