#!/bin/bash

BASENAME=`basename $0 .sh`
PATH_TO_TOKEN_1S=""
USERNAME_1S=""
PROXY="http://proxy-us.intel.com:912"
ONECCL_REPO="github.com/intel-innersource/libraries.performance.communication.oneccl.git"

print_help() {
    echo "Usage: ${BASENAME}.sh <options>"
    echo "  -t <token_path>"
    echo "      Path to file with github credentials"
    echo "  -u <name>"
    echo "      Github username with access to oneCCL repo"
    echo "  -p <url>"
    echo "      https proxy"
    echo ""
}

check_exit_code() {
    if [ $1 -ne 0 ]
    then
        echo "ERROR: ${2}"
        exit $1
    fi
}

run_cmd() {
    echo ${1}
    eval ${1}
    check_exit_code $? "cmd failed"
}

while [ $# -ne 0 ]
do
     case $1 in
            "-h" | "--help" | "-help")
                print_help
                exit 0
                ;;
            "-t")
                PATH_TO_TOKEN_1S=${2}
                shift
                ;;
            "-u")
                USERNAME_1S="${2}@"
                shift
                ;;
            "-p")
                PROXY="${2}"
                shift
                ;;
            *)
                echo "${BASENAME}: ERROR: unknown option ($1)"
                print_help
                exit 1
            ;;
     esac
     shift
done

rm -rf ccl

https_proxy=${PROXY} GIT_ASKPASS=${PATH_TO_TOKEN_1S} git clone https://${USERNAME_1S}${ONECCL_REPO} ccl

cd ccl
mkdir build
cd build
cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=dpcpp -DCOMPUTE_BACKEND=dpcpp_level_zero
make -j install

CCL_PATH=`pwd`/_install

source $CCL_PATH/env/setvars.sh
echo "CCL_ROOT: $CCL_ROOT"

scales="2 4 12"
algos="topo_a2a topo_ring ring"
copy_engines="none main link"
bench_params="-w 10 -i 16 -c all -t 262144 -j off"

base_env="CCL_ATL_TRANSPORT=ofi CCL_LOG_LEVEL=info"
base_env="${base_env} SYCL_DEVICE_FILTER=level_zero SYCL_PI_LEVEL_ZERO_BATCH_SIZE=1"

clinfo -l

for scale in ${scales}
do
    for algo in ${algos}
    do
        for copy_engine in ${copy_engines}
        do
            if [[ ${algo} = "ring" ]] && [[ ${copy_engine} != "none" ]]
            then
                continue
            fi

            exec_env="${base_env} CCL_ALLREDUCE=${algo} CCL_ZE_COPY_ENGINE=${copy_engine}"
            cmd="${exec_env} mpirun -n ${scale} -l ${CCL_PATH}/examples/benchmark/benchmark ${bench_params}"
            run_cmd "${cmd}"
        done
    done
done
