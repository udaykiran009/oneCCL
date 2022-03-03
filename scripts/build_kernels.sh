#!/bin/bash
KERNELS_PATH=$(dirname $0)/../src/kernels
echo "Building kernels in $KERNELS_PATH"

BASENAME=$(basename $0 .sh)

global_filename=""
has_a_option=false # compilation for all kernels
has_d_option=0 # compilation with debug logs. Default: disable
has_h_option=false # help

function print_help() {
    echo -e "Compile OpenCL kernels into SPIR-V binaries \n" \
            "Options:\n"\
            " [-a]:            Compile all .cl files in the current folder: ./${BASENAME}.sh -a\n" \
            " [filename]:      Name of the file to compile. Example: ./${BASENAME}.sh kernels.cl\n" \
            " [-d <enable/disable debug logs>]      by default ${has_d_option} - exmaple: -d 1\n" \
            " [-h]:            Print this help message\n\n" \
            "Examples: ./${BASENAME}.sh -a -d 0\n" \
            "          ./${BASENAME}.sh filename\n" \
            "          ./${BASENAME}.sh -d 1 filename\n" \
    exit 0
}

function parse_options() {
    while getopts :ad:h opt
    do
        case $opt in
            a) has_a_option=true ;; # build all kernels
            d) has_d_option=${OPTARG}
               if [ "$has_d_option" != "1" ] && [ "$has_d_option" != "0" ]
               then
                      echo "Error: -d option got an arg: $OPTARG. See help:"
                      print_help
                      exit 1
               fi
               ;; # build with debug logs
            h) print_help; exit ;;
            :) echo "Missing argument for option -$OPTARG, See help:";
               print_help
               exit 1;;
           \?) echo "Unknown option -$OPTARG";
               exit 1;;
        esac
    done

    shift $((OPTIND -1))
    global_filename=$1
}

# Verify that the specified command is available
function check_cmd() {
    if ! [ -x "$(command -v $1)" ]
    then
        echo "Error: $1 is not in PATH"
        exit 1
    fi
}

function run_cmd() {
    echo "$1"
    $1
}

function run_build() {
    local kernel_file_name=$1

    local defines=""

    # should be aligned with cmake function set_lp_env
    # defines+=" -DCCL_BF16_GPU_TRUNCATE"
    # defines+=" -DCCL_FP16_GPU_TRUNCATE"

    kernel="$KERNELS_PATH/$kernel_file_name"
    if [[ ! -f "$kernel" ]]
    then
        echo "Error: unknown kernel file: $kernel"
        exit 1
    fi

    if [ $has_d_option -eq 1 ]
    then
        echo "Add -DENABLE_KERNEL_DEBUG in compilation time"
        defines+=" -DENABLE_KERNEL_DEBUG"
    fi

    # Get full file name inluding path but without file extension
    fn=$(echo $kernel | sed 's/.cl$//g')

    # Check if the required commands are available before using them
    check_cmd clang
    check_cmd llvm-spirv

    # Use commands from https://www.khronos.org/blog/offline-compilation-of-opencl-kernels-into-spir-v-using-open-source-tooling
    # + some additional flags(-flto is used because clang complains when llvm-bc is emitted without the option)
    run_cmd "clang -cc1 ${defines} -triple spir64-unknown-unknown $fn.cl -O3 -flto -emit-llvm-bc -include opencl-c.h -x cl -cl-std=CL2.0 -o $fn.bc"
    run_cmd "llvm-spirv $fn.bc -o $fn.spv"
    run_cmd "rm -f $fn.bc"
}

function run_all_build() {
    # run_build already aware of kernel path, so just list the directory and
    # provide names of the kernels
    for f in `find $KERNELS_PATH -type f -name "*.cl" -printf "%f\n"`
    do
        echo "Building $f"
        run_build $f
    done
}

function main() {
    if $has_a_option
    then
        run_all_build
    elif [ -n "$global_filename" ]
    then
        run_build $global_filename
    else
        print_help
    fi
}

parse_options "$@"
main
