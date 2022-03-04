#include "common/api_wrapper/api_wrapper.hpp"
#include "common/api_wrapper/mpi_api_wrapper.hpp"

#if defined(CCL_ENABLE_MPI)

namespace ccl {

lib_info_t mpi_lib_info;
mpi_lib_ops_t mpi_lib_ops;

bool mpi_api_init() {
    bool ret = true;

    mpi_lib_info.ops = &mpi_lib_ops;
    mpi_lib_info.fn_names = mpi_fn_names;

    // lib_path specifies the name and full path to the MPI library
    // it should be absolute and validated path
    // pointing to desired libmpi library
    mpi_lib_info.path = ccl::global_data::env().mpi_lib_path;

    if (mpi_lib_info.path.empty()) {
        mpi_lib_info.path = "libmpi.so.12";
    }
    LOG_DEBUG("MPI lib path: ", mpi_lib_info.path);

    load_library(mpi_lib_info);
    if (!mpi_lib_info.handle)
        ret = false;

    return ret;
}

void mpi_api_fini() {
    LOG_DEBUG("close MPI lib: handle:", mpi_lib_info.handle);
    close_library(mpi_lib_info);
}

} //namespace ccl
#endif //CCL_ENABLE_MPI
