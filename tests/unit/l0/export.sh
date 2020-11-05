#!/bin/bash

SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
PROJECT_DIR="${SCRIPT_DIR}/../../../"
INCLUDE_PATH_TO_LEVEL_ZERO="/usr/local/include/level_zero"
LIB_PATH_TO_LEVEL_ZERO="/usr/local/lib/"
function show_help()
{
    echo -e "\t\tDescription: The current script provides a possibility to export\n\
            \tFROM kernel Unit tests TO independent kernel's test without CCL dependency.\n\n\
            \tUsage: export.sh -[options] dst_dir \n\n\
            \t\t-n: generate files for native api\n
            \t\t-s: generate files for SINGLE device \n\
            \t\t-m: generate files for MULTI device \n\
            \t\t-t: generate files for MULTI TILE device \n\
            \t\t-i: generate files for IPC \n\
            \t\t-g: generate files for TOPOLOGY (not implemented yet)\n\
            \t\t-p: make a tar archive with the final folder\n\
            \t\t-c: compile an example. Must launch with the specific mode, i.e.: \n\
            \t\t\tIt'll complie for single device\n\
            \t\t\texport.sh -c -s dst_dir OR export.sh -cs dst_dir\n\n\
            \t\t-h: show help\n\n\
            \t\tNote: For successful compiling It requires 'level_zero' include/lib paths.\n\
            \t\tIn order to change the paths, please, modify these variables in the script:\n\
            \t\tINCLUDE_PATH_TO_LEVEL_ZERO, LIB_PATH_TO_LEVEL_ZERO\n\n\n\
            \tIMPORTANT: launch this script from 'mlsl2/tests/unit/l0/' in case of grabbing data\n\
            \tSee some exaples to launch:\n\
            \t\t 1. export.sh -cs dst_dir, export.sh -cm dst_dir, export.sh -s dst_dir and etc.\n\
            \t\t 2. export.sh -csp dst_dir - compile single_device_suite and pack it into archive\n\
            \n\
            \tCompiling string: g++ -g -DMULTI_GPU_SUPPORT=1 -DSTANDALONE_UT -DUT_DEBUG -I${INCLUDE_PATH_TO_LEVEL_ZERO} -I./ -Ioneapi -Ioneapi/ccl native_device_api/l0/*.cpp \n\
            \t\t\t -L${LIB_PATH_TO_LEVEL_ZERO} -lze_loader -W -w -L/usr/lib/ -lpthread  -pthread ./input_name -o ./out_name\n"
    }

out_name=""
input_name=""
has_c_option=false # compile
has_s_option=false # single device
has_m_option=false # multi device
has_n_option=false # native api
has_t_option=false # multi tile device
has_i_option=false # ipc device
has_g_option=false # topology
has_p_option=false # pack
while getopts :hcasmntpg opt; do
    case $opt in
        h) show_help; exit ;;
        c) has_c_option=true ;; # compile
        s) has_s_option=true ;; # single device
        m) has_m_option=true ;; # multi device
        n) has_n_option=true ;; # native api
        t) has_t_option=true ;; # multi tile device
        i) has_i_option=true ;; # ipc device
        g) has_g_option=true ;; # topology
        p) has_p_option=true ;; # pack
        :) echo "Missing argument for option -$OPTARG"; exit 1;;
       \?) echo "Unknown option -$OPTARG"; exit 1;;
    esac
done

shift $((OPTIND -1))
export_dir=$1

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
        echo "Copy $dir -------> to destination: ${export_dir}/${dir_suffix}/${to_dir}"
        cp -Tr ${dir} "${export_dir}/${dir_suffix}/${to_dir}"
    done
}

function export_ccl_common()
{
     declare -a CCLPublicSourceArray=(
                            "${PROJECT_DIR}/include/oneapi/ccl/aliases.hpp"
                            "${PROJECT_DIR}/include/oneapi/ccl/exception.hpp"
                            "${PROJECT_DIR}/include/oneapi/ccl/string.hpp"
                            "${PROJECT_DIR}/include/oneapi/ccl/types.hpp"
                            "${PROJECT_DIR}/include/oneapi/ccl/type_traits.hpp"
                            "${PROJECT_DIR}/include/oneapi/ccl/device_types.hpp"
                            "${PROJECT_DIR}/include/oneapi/ccl/device_type_traits.hpp"
                            )
    copy_files "oneapi/ccl" ${CCLPublicSourceArray[@]}

    declare -a NAPIHeaderDirsArray=(
                                "${PROJECT_DIR}/include/oneapi/ccl/native_device_api/l0"
                                )
    copy_dirs "oneapi/ccl/native_device_api" ${NAPIHeaderDirsArray[@]}

    declare -a NAPISourceDirsArray=(
                                "${PROJECT_DIR}/src/native_device_api/l0"
                                )

    copy_dirs "native_device_api" ${NAPISourceDirsArray[@]}

    declare -a SpecialSources=("${PROJECT_DIR}src/native_device_api/api_explicit_instantiation.hpp")
    copy_files "oneapi/ccl/native_device_api" ${SpecialSources[@]}


    rm "${export_dir}/native_device_api/l0/export.cpp"
}

function export_test_common()
{
    declare -a SourceArray=("${SCRIPT_DIR}/base.hpp"
                            "${SCRIPT_DIR}/utils.hpp"
                            "${SCRIPT_DIR}/fixture.hpp"
                            "${SCRIPT_DIR}/base_fixture.hpp"
                            "${SCRIPT_DIR}/kernel_storage.hpp"
                            )
    cp "${PROJECT_DIR}/src/ccl_utils.cpp" "${export_dir}/native_device_api/l0/"

    echo "Copy common UT headers"
    copy_files "" ${SourceArray[@]}
}

function copy_all_stuff()
{
    printf "#define CCL_PRODUCT_FULL 0\n#define CCL_API\n#define MULTI_GPU_SUPPORT" > "${export_dir}/oneapi/ccl/config.h"
    mkdir -p "${export_dir}/kernels"
    declare -a TestSuite=("${SCRIPT_DIR}/kernels/*_test.hpp"
                          "${SCRIPT_DIR}/kernels/*_fixture.hpp")
    copy_files  "kernels" ${TestSuite[@]}

    find "${PROJECT_DIR}/src/kernels/" -type f -name "*.cl" \
          -o -name "*.spv" \
          -exec cp "{}" "${export_dir}/kernels/"  \;
}

function compile()
{
    cd ${export_dir}
    echo "Compiling..."
    # Note: -W -w disable g++ warnings
     output=$(g++ -g -DMULTI_GPU_SUPPORT=1 -DSTANDALONE_UT -DUT_DEBUG -I${INCLUDE_PATH_TO_LEVEL_ZERO} \
    -I./ -Ioneapi -Ioneapi/ccl native_device_api/l0/*.cpp -L${LIB_PATH_TO_LEVEL_ZERO} -lze_loader \
    -L/usr/lib/ -lpthread  -pthread \
    ./$input_name -o ./$out_name 2>&1)

    if [[ $? != 0 ]]; then
        # There was an error, display the error in $output
        echo -e "Error:\n$output"
    else
        echo -e "Compiling done"
    fi
}

function make_archive()
{
    echo "Start making archive from ${SCRIPT_DIR}/${export_dir}"
    cd "${SCRIPT_DIR}"
    output=$(tar cvfz export_dir.tar.gz ./${export_dir} 2>&1)
    if [[ $? != 0 ]]; then
        echo -e "Error:\n$output"
    else
        echo -e "Archive is done. Finale name of archive: export_dir.tar.gz"
    fi
}

function main()
{
    mkdir -p ${export_dir}
    mkdir -p "${export_dir}/oneapi/ccl/native_device_api/l0"
    mkdir -p "${export_dir}/native_device_api/l0"

    if $has_s_option; then
        echo "kernel_ring_single_device_suite test"
        out_name='kernel_ring_single_device_suite'

        declare -a TestSource=("${SCRIPT_DIR}/kernel_ring_single_device_suite.cpp" )
        copy_files "" ${TestSource[@]}

        input_name="kernel_ring_single_device_suite.cpp"
    fi

    if $has_m_option; then
        echo "kernel_ring_multi_device_suite"

        out_name="kernel_ring_multi_device_suite"
        declare -a TestSource=("${SCRIPT_DIR}/kernel_ring_multi_device_suite.cpp" )
        copy_files "" ${TestSource[@]}

        input_name="kernel_ring_multi_device_suite.cpp"
    fi

    if $has_n_option; then
        echo "native_api_suite"

        out_name="native_api_suite"
        declare -a TestSource=("${SCRIPT_DIR}/native_api_suite.cpp" )
        copy_files "" ${TestSource[@]}

        declare -a TestSuiteSource=("${SCRIPT_DIR}/native_api/platform_test.hpp"
                                    "${SCRIPT_DIR}/native_api/subdevice_test.hpp" )
        copy_files "native_api" ${TestSuiteSource[@]}
        input_name="native_api_suite.cpp"
    fi

    if $has_t_option; then
        echo "kernel_ring_single_device_multi_tile_suite test"
        out_name="kernel_ring_single_device_multi_tile_suite"

        declare -a TestSource=("${SCRIPT_DIR}/kernel_ring_single_device_multi_tile_suite.cpp" )
        copy_files "" ${TestSource[@]}

        input_name="kernel_ring_single_device_multi_tile_suite.cpp"
    fi

    if $has_i_option; then
        echo "kernels_ipc_test"
        out_name="kernels_ipc_test"

        declare -a TestSource=("${SCRIPT_DIR}/kernel_ipc_single_dev_suite.cpp")
        copy_files "" ${TestSource[@]}

        input_name="kernel_ipc_single_dev_suite.cpp"
    fi

    if $has_g_option; then
        echo "topology NOT IMPLEMENTED YET"
        exit 0
    fi

    copy_all_stuff
}

main
export_ccl_common
export_test_common

if $has_c_option; then
    compile
fi

if $has_p_option; then
    make_archive
fi
exit 0
