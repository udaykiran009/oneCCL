KERNELS_PATH=$(dirname $0)/../src/kernels
echo "Building kernels in $KERNELS_PATH"

BASENAME=$(basename $0 .sh)

# Verify that the specified command is available
check_cmd() {
    if ! [ -x "$(command -v $1)" ]; then
        echo "Error: $1 is not in PATH"
        exit 1
    fi
}

run_cmd() {
    echo "$1"
    $1
}

run_build() {
    kernel="$KERNELS_PATH/$1"
    if [[ ! -f "$kernel" ]]; then
        echo "Error: unknown kernel file: $kernel"
        exit 1
    fi

    # Get full file name inluding path but without file extension
    fn=$(echo $kernel | sed 's/.cl$//g')

    # Check if the required commands are available before using them
    check_cmd clang
    check_cmd llvm-spirv

    # should be aligned with cmake function set_lp_env
    defines="-DCCL_BF16_GPU_TRUNCATE"

    # Use commands from https://www.khronos.org/blog/offline-compilation-of-opencl-kernels-into-spir-v-using-open-source-tooling
    # + some additional flags(-flto is used because clang complains when llvm-bc is emitted without the option)
    run_cmd "clang -cc1 ${defines} -triple spir64-unknown-unknown $fn.cl -O3 -flto -emit-llvm-bc -include opencl-c.h -x cl -cl-std=CL2.0 -o $fn.bc"
    run_cmd "llvm-spirv $fn.bc -o $fn.spv"
}

run_build_all() {
    # run_build already aware of kernel path, so just list the directory and
    # provide names of the kernels
    for f in `find $KERNELS_PATH -type f -name "*.cl" -printf "%f\n"`; do
        echo "Building $f"
        run_build $f
    done
}

print_help() {
    echo "Compile OpenCL kernels into SPIR-V binaries"
    echo "Usage: ./${BASENAME}.sh [-a | --all] [filename] [-h | --help]"
    echo " -a | --all     Compile all .cl files in the current folder"
    echo " filename       Name of the file to compile. e.g ring_allreduce.cl"
    echo " -h | --help    Print this help message"
    exit 0
}


# Empty arguments is a special case to print help message
if [[ "$#" -eq 0 ]]; then
    print_help
fi

case $1 in
    -a|--all)
        run_build_all;
        ;;
    -h|--help)
        print_help;
        ;;
    *)
        run_build $1;
        ;;
esac

