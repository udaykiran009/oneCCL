#! /bin/bash

BUILD_CCL=0
RUN_CCL=1
RUN_MPI=1
RESNET_ON=0

msg_count_list=2097152
msg_count_list_resnet="2048000,1048576,2359296,1048576,1048576,2359296,1048576,1048576,2359296,524288,2097152,262144,589824,262144,262144,589824,262144,262144,589824,262144,262144,589824,262144,262144,589824,262144,262144,589824,131072,524288,65536,147456,65536,65536,147456,65536,65536,147456,65536,65536,147456,32768,131072,16384,36864,16384,16384,36864,16384,16384,36864,4096,16384,9408"
msg_log_start=23
msg_log_end=23
iters=10000
ccl_coll_list="allreduce"
# ccl_coll_list="allgatherv allreduce alltoall bcast reduce reduce_scatter"
mpi_coll_list="allgatherv allreduce alltoall bcast reduce_scatter" # skip reduce since it takes too much time

WORK_DIR="/home/gta/ksenyako/test_bench"
# COMPILER_PATH="/home/gta/ksenyako/intel/oneapi/compiler/latest"
COMPILER_PATH="/home/sys_ctlab/jenkins/oneapi/compiler/install/compiler/2021.3.0/"
#COMPILER_PATH="/home/gta/ksenyako/l_compiler_p_2021.2.310/compiler/2021.2.0"

CCL_PATH="${WORK_DIR}/mlsl2/build/_install"
CCL_PATH_RELEASED="/home/gta/intel/ccl_2021.2.0"
CCL_URL="https://github.intel.com/ict/mlsl2.git"
CCL_BRANCH="master"
BENCH_CCL_PATH="${CCL_PATH}/examples/benchmark"

#IMPI_PATH="/home/gta/ksenyako/ww49_20201204" - last good known version for agama 372
#IMPI_PATH="/home/gta/intel/impi_2021.2.0/"
# IMPI_PATH="/home/gta/ksenyako/intel/impi_2021.2.1"
IMPI_PATH="/home/sys_ctlab/jenkins/oneapi/compiler/install/mpi/2021.3.0/"
IMB_GPU_PATH="${IMPI_PATH}/bin/IMB-MPI1-GPU"

common_log="${WORK_DIR}/common.log"
ccl_l0_raw_log="${WORK_DIR}/ccl_l0_raw.log"
released_ccl_l0_raw_log="${WORK_DIR}/released_ccl_l0_raw.log"
ccl_ocl_raw_log="${WORK_DIR}/ccl_ocl_raw.log"
ccl_l0_scale_out_raw_log="${WORK_DIR}/ccl_l0_scale_out_raw.log"
ccl_l0_parsed_log="${WORK_DIR}/ccl_l0_parsed.log"
released_ccl_l0_parsed_log="${WORK_DIR}/released_ccl_l0_parsed.log"
ccl_ocl_parsed_log="${WORK_DIR}/ccl_ocl_parsed.log"
ccl_l0_scale_out_parsed_log="${WORK_DIR}/ccl_l0_scale_out_parsed.log"
mpi_raw_log="${WORK_DIR}/mpi_raw.log"
mpi_parsed_log="${WORK_DIR}/mpi_parsed.log"

ccl_ocl_scale_out_raw_log="${WORK_DIR}/ccl_ocl_scale_out_raw.log"
ccl_ocl_scale_out_parsed_log="${WORK_DIR}/ccl_ocl_scale_out_parsed.log"

run_ccl()
{
    echo "================ENVIRONMENT=================="
    local base_env="$1"
    echo "base_env: "$base_env
    local extra_env="$2"
    echo "extra_env: "$extra_env
    local raw_log="$3"
    echo "raw_log: "$raw_log
    local parsed_log="$4"
    echo "parsed_log: "$parsed_log
    echo "================ENVIRONMENT=================="
    for coll in ${ccl_coll_list}
    do
        cmd=`echo ${base_env} ${extra_env} mpiexec -n 2 ${BENCH_CCL_PATH}/benchmark  -l ${coll} -i ${iters} -y ${msg_count_list} -p 1 -n 1 -b sycl`
        echo "running: $cmd" | tee -a ${raw_log}
        eval $cmd 2>&1 | tee -a ${raw_log}
    done
}

parse_ccl()
{
    local raw_log="$1"
    local parsed_log="$2"
    for coll in ${ccl_coll_list}
    do
        timing=`cat ${raw_log} | grep -i "Benchmarking: ${coll} " -A 5 | tail -n 1 | awk '{ print $4 }'`
        echo "${coll}: ${timing}" | tee -a ${parsed_log}
    done
}

run_mpi()
{
    echo "================ENVIRONMENT=================="
    local base_env="$1"
    echo "base_env: "$mpi_env
    local extra_env="$2"
    echo "extra_env: "$extra_env
    local raw_log="$3"
    echo "raw_log: "$raw_log
    local parsed_log="$4"
    echo "parsed_log: "$parsed_log
    echo "================ENVIRONMENT=================="
    for coll in ${ccl_coll_list}
    do
        cmd=`echo ${mpi_env} mpiexec -n 2 ${IMB_GPU_PATH} ${coll} -mem_alloc_type device -npmin 1024 -msglog ${msg_log_start}:${msg_log_end} -zero_size 0 -iter ${iters},2047`
        echo "running: $cmd" | tee -a ${raw_log}
        eval $cmd 2>&1 | tee -a ${parsed_log}
    done
}

parse_mpi()
{
    local raw_log="$1"
    local parsed_log="$2"
    for coll in ${ccl_coll_list}
    do
        timing=`cat ${raw_log} | grep -i "Benchmarking ${coll} " -A 4 | tail -n 1 | awk '{ print $4 }'`
        echo "${coll}: ${timing}" | tee -a ${parsed_log}
    done
}


mkdir -p ${WORK_DIR}
cd ${WORK_DIR}
rm ${common_log} ${ccl_raw_log} ${ccl_parsed_log} ${mpi_raw_log} ${mpi_parsed_log}

IMPI_VARS_SCRIPT="${IMPI_PATH}/env/vars.sh"
if [ ! -f "${IMPI_VARS_SCRIPT}" ]
then
    IMPI_VARS_SCRIPT="${IMPI_PATH}/intel64/bin/mpivars.sh"
fi

if [ ! -f "${IMPI_VARS_SCRIPT}" ]
then
    echo "can not find IMPI vars script: ${IMPI_VARS_SCRIPT}"
    exit 0
fi

source ${COMPILER_PATH}/env/vars.sh intel64

if [ "${BUILD_CCL}" = "1" ]
then
    rm -r ./mlsl2
    git clone -b ${CCL_BRANCH} --single-branch ${CCL_URL}
    cd ./mlsl2
    mkdir build
    cd build
    cmake .. -DBUILD_FT=0 -DBUILD_UT=0 -DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=dpcpp
    make -j12 install
fi

source ${CCL_PATH}/env/vars.sh --ccl-configuration=cpu_gpu_dpcpp
source ${IMPI_VARS_SCRIPT} --i_mpi_library_kind=release
if [ "${RESNET_ON}" == "0" ]
then
    base_env="I_MPI_ADJUST_ALLREDUCE=14 CCL_ATL_SYNC_COLL=1  I_MPI_DEBUG=12 I_MPI_PIN_PROCESSOR_LIST=1,2"
else
    msg_count_list="${msg_count_list_resnet}"
    base_env="I_MPI_DEBUG=12 I_MPI_PIN_PROCESSOR_LIST=1,2"
fi
base_env="${base_env} CreateMultipleSubDevices="
base_env="${base_env} xCCL_WORKER_COUNT=8 CCL_WORKER_AFFINITY=3,4,5,6 CCL_LOG_LEVEL=info"
base_env="${base_env} FI_PROVIDER=tcp"

mpi_env="${base_env} I_MPI_OFFLOAD=2 I_MPI_OFFLOAD_TOPOLIB=l0 I_MPI_OFFLOAD_CBWR=0"
mpi_env="${mpi_env} I_MPI_OFFLOAD_QUEUE_CACHE=1 I_MPI_OFFLOAD_LIST_CACHE=1"

if [ "${RUN_CCL}" == "0" ]
then
    ccl_coll_list=""
fi

if [ "${RUN_MPI}" == "0" ]
then
    mpi_coll_list=""
fi

delim="###############################"

echo -e "${delim}\nCPU information:\n" | tee -a ${common_log}
lscpu | tee -a ${common_log}

echo -e "${delim}\nKernel information:\n" | tee -a ${common_log}
uname -a | tee -a ${common_log}

echo -e "${delim}\nGPU SW stack information:\n" | tee -a ${common_log}
dpkg --list | grep level-zero | tee -a tee ${common_log}

echo -e "${delim}\nMPI information:\n" | tee -a ${common_log}
mpiexec -V | tee -a ${common_log}
# launch last released oneCCL
echo -e "\nstart released CCL level zero test\n"
(
    source ${CCL_RELEASED_PATH}/env/vars.sh --ccl-configuration=cpu_gpu_dpcpp
    extra_env="xCCL_STAGING_BUFFER=regular SYCL_DEVICE_FILTER=level_zero:*:0"
    run_ccl "${base_env}" "${extra_env}" ${released_ccl_l0_raw_log} ${released_ccl_l0_parsed_log}
)
echo -e "\nstart CCL level zero test\n"
extra_env="xCCL_STAGING_BUFFER=regular SYCL_DEVICE_FILTER=level_zero:*:0"
run_ccl "${base_env}" "${extra_env}" ${ccl_l0_raw_log} ${ccl_l0_parsed_log}

echo -e "\nstart CCL opencl test\n"
extra_env="xCCL_STAGING_BUFFER=regular SYCL_DEVICE_FILTER=opencl:*:0"
run_ccl "${base_env}" "${extra_env}" ${ccl_ocl_raw_log} ${ccl_ocl_parsed_log}

echo -e "\nstart CCL opencl scale outtest\n"
extra_env="CCL_STAGING_BUFFER=regular SYCL_DEVICE_FILTER=opencl:*:0"
run_ccl "${base_env}" "${extra_env}" ${ccl_ocl_scale_out_raw_log} ${ccl_ocl_scale_out_parsed_log}

echo -e "\nstart CCL level zero scale out test\n"
extra_env="CCL_STAGING_BUFFER=regular SYCL_DEVICE_FILTER=level_zero:*:0"
run_ccl "${base_env}" "${extra_env}" ${ccl_l0_scale_out_raw_log} ${ccl_l0_scale_out__parsed_log}

echo -e "\nstart MPI test\n"
extra_env=""
run_mpi "${mpi_env}" "${extra_env}" ${mpi_raw_log} ${mpi_parsed_log}


echo -e "\nstart released CCL level zero parsing\n"
parse_ccl  ${released_ccl_l0_raw_log} ${released_ccl_l0_parsed_log}

echo -e "\nstart CCL level zero parsing\n"
parse_ccl ${ccl_l0_raw_log} ${ccl_l0_parsed_log}

echo -e "\nstart CCL opencl parsing\n"
parse_ccl ${ccl_ocl_raw_log} ${ccl_ocl_parsed_log}

echo -e "\nstart CCL level zero scale out parsing\n"
parse_ccl ${ccl_l0_scale_out_raw_log} ${ccl_l0_scale_out_parsed_log}

echo -e "\nstart CCL opencl scale out parsing\n"
parse_ccl ${ccl_ocl_scale_out_raw_log} ${ccl_ocl_scale_out_parsed_log}

echo -e "\nstart MPI parsing\n"
parse_mpi ${mpi_raw_log} ${mpi_parsed_log}

echo -e "\ncompleted\n"

#nccl testing on endv

# ssh 207.108.8.122
# bsub -P R -Is -t 100 -N 1 -p cnvq /bin/bash
# git clone https://github.com/NVIDIA/nccl.git
# cd nccl
# make -j src.build CUDA_HOME=/opt/crtdc/cuda/11.3
# make pkg.debian.build
# git clone https://github.com/NVIDIA/nccl-tests.git
# cd nccl-tests
# make  NCCL_HOME=/panfs/users/ksenyako/nccl_experiments/nccl/build/ CUDA_HOME=/opt/crtdc/cuda/11.3 
# in case you want enable MPI in NCCL tests run
# make NCCL_HOME=<nccl_path> CUDA_HOME=<cuda_path> MPI=1 MPI_HOME=<mpi_path>
# export LD_LIBRARY_PATH=/panfs/users/ksenyako/nccl_experiments/nccl/build/lib/:$LD_LIBRARY_PATH                                        ldd ./build/all_reduce_perf
# ./build/all_reduce_perf -b 8 -e 256M -f 2 -g 1
# to run with mpirun on Endeavour cluster nodes with Nvidia cards you might need a workaround to run processes like 1 per 1 GPU
# mpirun -bootstrap ssh -n 2 -ppn 1 -hosts-group `hostname` ./build/all_reduce_perf -b 8 -e 256M -f 2 -g 1