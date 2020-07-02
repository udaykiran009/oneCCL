#include <limits.h>
#include <unistd.h>

#include "ccl_gpu_modules.h"

#ifdef MULTI_GPU_SUPPORT
#include "common/comm/l0/modules/specific_modules_source_data.hpp"
#include "common/comm/l0/device_group_routing_schema.hpp"

ccl_status_t CCL_API register_allreduce_gpu_module_source(const char* path, ccl_topology_class_t topology_class)
{
    ccl::device_topology_type t_class = static_cast<ccl::device_topology_type>(topology_class);
    char pwd[PATH_MAX];
    char* ret = getcwd(pwd, sizeof(pwd));
    (void)ret;

    LOG_INFO("loading file contained gpu module \"", ccl_coll_type_to_str(ccl_coll_allreduce),
             "\", topology class: \"", to_string(t_class), "\" by path: ", path, ". Current path is: ", pwd);

    try
    {
        if (!path)
        {
            throw std::runtime_error("Path is empty");
        }
        native::specific_modules_source_data_storage::instance().load_kernel_source<ccl_coll_allreduce>(path, t_class);
    }
    catch(const std::exception &ex)
    {
        LOG_ERROR("Cannot preload kernel source by path: ", path, ", error: ", ex.what());
        CCL_ASSERT(false);
        return ccl_status_runtime_error;
    }

    LOG_INFO("gpu kernel source by type \"", ccl_coll_type_to_str(ccl_coll_allreduce),
              "\", topology class: \"", to_string(t_class), "\" loaded succesfully");
    return ccl_status_success;
}
#endif //MULTI_GPU_SUPPORT
