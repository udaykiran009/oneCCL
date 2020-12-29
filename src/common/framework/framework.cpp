#include "common/framework/framework.hpp"

ccl_horovod_init_function horovod_init_function = nullptr;

std::map<ccl_framework_type, std::string> ccl_framework_type_names = {
    std::make_pair(ccl_framework_none, "none"),
    std::make_pair(ccl_framework_horovod, "horovod")
};
