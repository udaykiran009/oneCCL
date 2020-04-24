#!/bin/bash

SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
PROJECT_DIR="${SCRIPT_DIR}/../../../"
#echo $SCRIPT_DIR

testname=$1
export_dir=$2
echo "Testname: ${testname} export: ${export_dir}"

function copy_files()
{
    dir_suffix=$1
    shift
    list=("$@")

    mkdir -p "${export_dir}/${dir_suffix}"

    for file in "${list[@]}";
    do
        to_file=`basename "${file}"`
        #echo "Copy $file to destination: ${export_dir}/${dir_suffix}/${to_file}"
        cp ${file} "${export_dir}/${dir_suffix}/${to_file}"
    done
}

function copy_dirs()
{
    dir_suffix=$1
    shift
    list=("$@")

    mkdir -p "${export_dir}/${dir_suffix}"

    for dir in "${list[@]}";
    do
        to_dir=`basename "${dir}"`
        echo "Copy $dir to destination: ${export_dir}/${dir_suffix}/${to_dir}"
        cp -Tr ${dir} "${export_dir}/${dir_suffix}/${to_dir}"
    done
}

function export_ccl_common()
{
    declare -a SourceArray=("${PROJECT_DIR}/include/native_device_api/export_api.hpp"
                            "${PROJECT_DIR}/include/ccl_types.hpp"
                            "${PROJECT_DIR}/include/ccl_types.h"
                            "${PROJECT_DIR}/include/ccl.hpp"
                            "${PROJECT_DIR}/include/gpu_communicator.hpp"
                            "${PROJECT_DIR}/include/ccl_config.h"
                            "${PROJECT_DIR}/include/ccl_type_traits.hpp"
                            )
    echo "Copy ccl library headers"
    copy_files "" ${SourceArray[@]}

    declare -a SourceDirsArray=("${PROJECT_DIR}/include/native_device_api"
                                "${PROJECT_DIR}/src/native_device_api")

    echo "Copy ccl library directories"
    copy_dirs "" ${SourceDirsArray[@]}

#    declare -a SpecialSources=("${PROJECT_DIR}/src/exec/l0/compiler_ccl_wrappers_dispatcher.hpp"
                            #)
    #copy_files "exec/l0/" ${SpecialSources[@]}
}

function export_test_common()
{
    declare -a SourceArray=("${SCRIPT_DIR}/base.hpp"
                            "${SCRIPT_DIR}/utils.hpp"
                            "${SCRIPT_DIR}/fixture.hpp"
                            "${SCRIPT_DIR}/base_fixture.hpp"
                            )
    echo "Copy common UT headers"
    copy_files "" ${SourceArray[@]}
}

mkdir -p ${export_dir}
case ${testname} in
    'kernels_single_dev')
        echo "kernels single dev test"

        declare -a TestSource=("${SCRIPT_DIR}/kernel_single_dev_suite.cpp" )
        copy_files "" ${TestSource[@]}

        mkdir -p "${export_dir}/kernels"
        declare -a TestSuite=("${SCRIPT_DIR}/kernels/allreduce_test.hpp")
        copy_files  "kernels" ${TestSuite[@]}
        cp "${PROJECT_DIR}/src/kernels/ring_allreduce.cl"  "${export_dir}/kernels/"
        cp "${PROJECT_DIR}/src/kernels/ring_allreduce.spv"  "${export_dir}/kernels/"
    ;;
    'kernels_ipc')
    echo "kernels ipc test"

        declare -a TestSource=("${SCRIPT_DIR}/kernel_ipc_single_dev_suite.cpp")
        copy_files "" ${TestSource[@]}

        mkdir -p "${export_dir}/kernels"
        declare -a TestSuite=("${SCRIPT_DIR}/kernels/ipc_single_dev_allreduce_test.hpp"
                              "${SCRIPT_DIR}/kernels/ipc_fixture.hpp")
        copy_files  "kernels" ${TestSuite[@]}
        cp "${PROJECT_DIR}/src/kernels/ring_allreduce.cl"  "${export_dir}/kernels/"
        cp "${PROJECT_DIR}/src/kernels/ring_allreduce.spv"  "${export_dir}/kernels/"
    ;;
    'topology')
        echo "topologies NOT IMPLEMENTED"
    ;;
    'kernels_multi_tile')
        echo "kernels_multi_tile test"
        declare -a TestSource=("${SCRIPT_DIR}/kernel_multi_tile_suite.cpp" )
        copy_files "" ${TestSource[@]}

        mkdir -p "${export_dir}/kernels"
        declare -a TestSuite=("${SCRIPT_DIR}/kernels/allreduce_multi_tile_case.hpp")
        copy_files  "kernels" ${TestSuite[@]}
        cp "${PROJECT_DIR}/src/kernels/ring_allreduce.cl"  "${export_dir}/kernels/"
        cp "${PROJECT_DIR}/src/kernels/ring_allreduce.spv"  "${export_dir}/kernels/"
    ;;
esac

export_ccl_common
export_test_common


#echo "Use to compile:\n  g++ -DSTANDALONE_UT -I/usr/include/level_zero -I *.cpp -L/usr/lib/ -llevel_zero -lpthread"
echo "Use to compile:\n clang++ -DSTANDALONE_UT -DUT_DEBUG -I/usr/include/level_zero -I./ -I/p/pdsd/scratch/sivanov/dpcpp/build/linux_prod/compiler/linux/lib/clang/10.0.0/include  *.cpp  native_device_api/l0/*.cpp -L/usr/lib/ -llevel_zero -lpthread"
exit 0
