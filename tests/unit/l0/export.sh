#!/bin/bash

SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
PROJECT_DIR="${SCRIPT_DIR}/../../../"

function show_help()
{
    echo -e "\t\tDescription: The current script provides a possibility to export\n\
            \tFROM kernel Unit tests TO independent kernel's test without CCL dependency.\n\n\
            \tUsage: export.sh -[options] dst_dir \n\n\
            \t\t-s: generate files for SINGLE device \n\
            \t\t-m: generate files for MULTI device \n\
            \t\t-t: generate files for MULTI TILE device \n\
            \t\t-p: generate files for IPC \n\
            \t\t-g: generate files for TOPOLOGY (not implemented yet)\n\
            \t\t-c: compile an example. Must launch with the specific mode, i.e.: \n\
            \t\t\tit'll complie for single device\n\
            \t\t\texport.sh -c -s dst_dir OR export.sh -cs dst_dir\n\n\
            \t\t-h: show help\n\n\
            \t\tNote: For successful compiling It requires 'level_zero' include/lib paths.\n\
            \t\tIn order to change the paths, please, modify these variables in the script:\n\
            \t\tINCLUDE_PATH_TO_LEVEL_ZERO, LIB_PATH_TO_LEVEL_ZERO\n\n\n\
            \tIMPORTANT: launch this script from 'mlsl2/tests/unit/l0/'\n\
            \tSee some exaples to launch:\n\
            \texport.sh -cs dst_dir, export.sh -cm dst_dir, export.sh -s dst_dir and etc.\n"
    }

out_name=""
input_name=""
has_c_option=false
has_s_option=false
has_m_option=false
has_t_option=false
has_p_option=false
has_g_option=false
while getopts :hcasmtpg opt; do
    case $opt in
        h) show_help; exit ;;
        c) has_c_option=true ;; # compile
        s) has_s_option=true ;; # single device
        m) has_m_option=true ;; # multi device
        t) has_t_option=true ;; # multi tile device
        p) has_p_option=true ;; # ipc device
        g) has_g_option=true ;; # topology
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
        echo "Copy $dir to destination: ${export_dir}/${dir_suffix}/${to_dir}"
        cp -Tr ${dir} "${export_dir}/${dir_suffix}/${to_dir}"
    done
}

function export_ccl_common()
{
    declare -a SourceArray=("${PROJECT_DIR}/include/native_device_api/export_api.hpp"
                            "${PROJECT_DIR}/include/ccl_types.hpp"
                            "${PROJECT_DIR}/include/ccl_types.h"
                            "${PROJECT_DIR}/include/ccl_type_traits.hpp"
                            "${PROJECT_DIR}/include/ccl_device_types.hpp"
                            "${PROJECT_DIR}/include/ccl_device_types.h"
                            "${PROJECT_DIR}/include/ccl_device_type_traits.hpp"
                            "${PROJECT_DIR}/include/ccl.hpp"
                            "${PROJECT_DIR}/include/ccl_config.h"
                            "${PROJECT_DIR}/include/ccl_gpu_modules.h"
                            "${PROJECT_DIR}/include/gpu_communicator.hpp"
                            )
    echo "Copy ccl library headers"
    copy_files "" ${SourceArray[@]}

    declare -a SourceDirsArray=("${PROJECT_DIR}/include/native_device_api"
                                "${PROJECT_DIR}/src/native_device_api")

    echo "Copy ccl library directories"
    copy_dirs "" ${SourceDirsArray[@]}

    # declare -a SpecialSources=("${PROJECT_DIR}/src/exec/l0/compiler_ccl_wrappers_dispatcher.hpp"
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
    cp "${PROJECT_DIR}/src/ccl_utils.cpp" "${export_dir}/native_device_api/l0/"
    echo "Copy common UT headers"
    copy_files "" ${SourceArray[@]}
}

function copy_all_stuff()
{
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
    g++ -g -DSTANDALONE_UT -DUT_DEBUG -I/usr/local/include/level_zero \
    -I./ native_device_api/l0/*.cpp -L/usr/local/lib/ -lze_loader \
    -L/usr/lib/ -lpthread  -pthread \
    ./$input_name -o ./$out_name
}

function main()
{
    mkdir -p ${export_dir}

    if $has_s_option; then
        echo "kernels_single_dev test"
        out_name='kernels_single_dev'

        declare -a TestSource=("${SCRIPT_DIR}/kernel_single_dev_suite.cpp" )
        copy_files "" ${TestSource[@]}

        input_name="kernel_single_dev_suite.cpp"
        # TODO: copy specific stuff
        copy_all_stuff
    fi

    if $has_m_option; then
        echo "kernels_multi_dev_test"
        echo -e "\tWarning: Multi device suite requires optional changes:\n\
                  i.e. namespace issue (add them like into kernel_single_dev_suite.cpp)\n\
                  Fixed them before !COMPILING!
                  But the files, you want for compiling, will be grabbed into the folder."
        out_name="kernels_multi_dev"
        declare -a TestSource=("${SCRIPT_DIR}/kernel_multi_dev_suite.cpp" )
        copy_files "" ${TestSource[@]}

        input_name="kernel_multi_dev_suite.cpp"
        copy_all_stuff
    fi

    if $has_t_option; then
        echo "kernels_multi_tile test"
        out_name="kernels_multi_tile"

        declare -a TestSource=("${SCRIPT_DIR}/kernel_multi_tile_suite.cpp" )
        copy_files "" ${TestSource[@]}

        input_name="kernel_multi_tile_suite.cpp"
        copy_all_stuff
    fi

    if $has_p_option; then
        echo "kernels_ipc_test"
        out_name="kernels_ipc_test"

        declare -a TestSource=("${SCRIPT_DIR}/kernel_ipc_single_dev_suite.cpp")
        copy_files "" ${TestSource[@]}

        input_name="kernel_ipc_single_dev_suite.cpp"
        copy_all_stuff
    fi

    if $has_g_option; then
        echo "topology NOT IMPLEMENTED YET"
        exit 0
    fi
}

main
export_ccl_common
export_test_common

if $has_c_option; then
    compile
fi

exit 0
