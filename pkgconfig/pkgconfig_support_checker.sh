#!/bin/bash

##########################################################################################
#CCL# Title         : Simple pkg-config support checker                                 #
#CCL# Tracker ID    : INFRA-1459                                                        #
#CCL#                                                                                   #
#CCL# Implemented in: Intel(R) Collective Communication Library 2021.4 (Gold) Update  4 #
##########################################################################################

cat <<"EOT" >"./test.cpp"
#include "oneapi/ccl.hpp"
#include <stdio.h>

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
using namespace cl::sycl;
using namespace cl::sycl::access;
#endif // CCL_ENABLE_SYCL

int main() {
    auto version = ccl::get_library_version();
    printf("\nCompile-time CCL library version:\nmajor: %d\nminor: %d\nupdate: %d\n"
   "Product: %s\nBuild date: %s\nFull: %s\n",
    CCL_MAJOR_VERSION,
    CCL_MINOR_VERSION,
    CCL_UPDATE_VERSION,
    CCL_PRODUCT_STATUS,
    CCL_PRODUCT_BUILD_DATE,
    CCL_PRODUCT_FULL);

    printf("\nPASSED\n");
    return 0;
}
EOT

# Setup 0 to enable debug mode.
debug_mode=1
echo_debug()
{
    if [ ${debug_mode} -eq 0 ]; then echo "DEBUG: $*"; fi
}

product="ccl"
mpi_dependency="mpi-release_mt"
testlog=$(basename $0 .sh).log
if [ -n "$1" ]; then conf="$1"; else conf="cpu_icc"; fi
echo_debug "configuration = ${conf}"

which pkg-config >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "ERROR: Required pkg-config, exit"; exit 2
fi

pkg-config --list-all | grep ${product} >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "ERROR: Not found ${product} pc files, check PKG_CONFIG_PATH, exit"; exit 2
fi

pkg-config --list-all | grep ${mpi_dependency} >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "ERROR: Missed MPI dependency (${mpi_dependency}.pc), check PKG_CONFIG_PATH, exit"; exit 2
fi

rm -rf ./test_cpp
gxx_line="g++ -o ./test_cpp -std=c++11 $(pkg-config --libs --cflags ${product}-${conf}) ./test.cpp"
echo_debug "${gxx_line}"
${gxx_line}; res_gxx=$?

(
    . /p/pdsd/scratch1/jenkins/artefacts/impi-ch4-nightly/last/oneapi_impi/env/vars.sh --i_mpi_library_kind=release_mt
    . /p/pdsd/scratch1/jenkins/artefacts/ccl-nightly/last/l_ccl_release*/env/vars.sh --ccl-configuration=cpu_icc
    ./test_cpp
)
res_gxx_run=$?

rm -rf ./test_cpp ./test.cpp
if [ $res_gxx -eq 0 ] && [ $res_gxx_run -eq 0  ]; then
    echo "Pass"
    exit 0
else
    echo "Fail"
    exit 2
fi
