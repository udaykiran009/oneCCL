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

run_cmd() {
    echo ${1}
    eval ${1}
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

rm -rf oneccl

https_proxy=${PROXY} GIT_ASKPASS=${PATH_TO_TOKEN_1S} git clone https://${USERNAME_1S}${ONECCL_REPO} oneccl
cd oneccl
mkdir build
cd build

cmake .. -DBUILD_FT=0 -DBUILD_EXAMPLES=1 -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=dpcpp -DCOMPUTE_BACKEND=dpcpp_level_zero -DMULTI_GPU_SUPPORT=1 && make -j install

export SYCL_DEVICE_FILTER=level_zero 
export SYCL_PI_LEVEL_ZERO_BATCH_SIZE=1

clinfo -l

CCL_PATH=./_install
source $CCL_PATH/env/setvars.sh

echo $CCL_ROOT
export CCL_ATL_TRANSPORT=ofi
#export CCL_LOG_LEVEL=info

affinity="0,1 0.0,1.0,2.0,3.0" #MDFI+ANR usage, ANR only useage
allreduce="topo_ring ring"
copy_ops="0 1"
copy_engine="none main link"
mpirun_cmd="mpirun -n 4 -l $CCL_PATH/examples/benchmark/benchmark -w 10 -i 16 -c last -t 262144 -j off"

for mask in ${affinity}
do
  for algo in ${allreduce}
  do
     cmd="ZE_AFFINITY_MASK=$mask CCL_ALLREDUCE=$algo"
     if [[ ${algo} = "topo_ring" ]]; then
        for c_op in ${copy_ops}
        do
             c_op_cmd="CCL_KERNEL_1S_USE_COPY_OPS=$c_op"
             if [[ ${c_op} = "1" ]]; then
                for eng in ${copy_engine}
                do
                  run_cmd "$cmd $c_op_cmd CCL_ZE_COPY_ENGINE=$eng $mpirun_cmd"
                done
             else
                run_cmd "$cmd $c_op_cmd $mpirun_cmd"
             fi
        done
     else
        run_cmd "$cmd $mpirun_cmd"
     fi     
  done
done
