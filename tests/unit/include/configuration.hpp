#pragma once

#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#define UT_DEBUG

#ifdef UT_DEBUG
static std::ostream& global_output = std::cout;
#else
static std::stringstream global_ss;
static std::ostream& global_output = global_ss;
#endif

static const char* ut_device_affinity_mask_name = "L0_CLUSTER_AFFINITY_MASK";
static std::string device_indices{ "[0:6459]" };

inline void set_test_device_indices(const char* indices_csv) {
    if (indices_csv) {
        device_indices = indices_csv;
    }
}

inline const std::string& get_global_device_indices() {
    return device_indices;
}
