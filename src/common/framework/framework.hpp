#pragma once

#include <map>

typedef int (*ccl_horovod_init_function)(const int*, int);
extern ccl_horovod_init_function horovod_init_function;
static constexpr const char* horovod_init_function_name = "horovod_init";

enum ccl_framework_type {
    ccl_framework_none,
    ccl_framework_horovod,

    ccl_framework_last_value
};

extern std::map<ccl_framework_type, std::string> ccl_framework_type_names;
