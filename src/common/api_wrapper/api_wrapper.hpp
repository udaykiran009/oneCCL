#pragma once

#include "oneapi/ccl/config.h"

#include "common/global/global.hpp"
#include "common/log/log.hpp"

namespace ccl {

typedef struct lib_info {
    std::string path;
    void* handle;
    void* ops;
    std::vector<std::string> fn_names;
} lib_info_t;

void api_wrappers_init();
void api_wrappers_fini();

void load_library(lib_info_t& info);
void close_library(lib_info_t& info);

} //namespace ccl
