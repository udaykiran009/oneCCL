#! /bin/bash

BUILD_CCL=0
RUN_CCL=1
RUN_MPI=1

msg_count=2097152
msg_log_start=23
msg_log_end=23
iters=100

ccl_coll_list="allgatherv allreduce alltoall bcast reduce reduce_scatter"
mpi_coll_list="allgatherv allreduce alltoall bcast reduce_scatter" # skip reduce since it takes too much time

WORK_DIR="/home/gta/ccl_mpi_perf"
COMPILER_PATH="/home/gta/oneapigold/compiler/latest"
#COMPILER_PATH="/home/gta/ksenyako/l_compiler_p_2021.2.310/compiler/2021.2.0"

CCL_PATH="${WORK_DIR}/mlsl2/build/_install"
CCL_URL="https://github.intel.com/ict/mlsl2.git"
CCL_BRANCH="master"

#IMPI_PATH="/home/gta/ksenyako/ww49_20201204" - last good known version for agama 372
#IMPI_PATH="/home/gta/intel/impi_2021.2.0/"
IMPI_PATH="/home/gta/mshiryae/impi_2019u10"
IMB_GPU_PATH="/home/gta/ksenyako/IMB-MPI1-GPU"

common_log="${WORK_DIR}/common.log"
ccl_raw_log="${WORK_DIR}/ccl_raw.log"
ccl_parsed_log="${WORK_DIR}/ccl_parsed.log"
mpi_raw_log="${WORK_DIR}/mpi_raw.log"
mpi_parsed_log="${WORK_DIR}/mpi_parsed.log"

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
    cmake .. -DBUILD_FT=0 -DBUILD_UT=0 -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=dpcpp -DCOMPUTE_BACKEND=dpcpp
    make -j12 install
fi

source ${CCL_PATH}/env/setvars.sh --ccl-configuration=cpu_gpu_dpcpp
source ${IMPI_VARS_SCRIPT} --i_mpi_library_kind=release

base_env="I_MPI_ADJUST_ALLREDUCE=14 I_MPI_DEBUG=12 I_MPI_PIN_PROCESSOR_LIST=1,2"
base_env="${base_env} SYCL_DEVICE_FILTER=level_zero:*:0 SYCL_BE=PI_LEVEL_ZERO CreateMultipleSubDevices="
base_env="${base_env} CCL_WORKER_AFFINITY=3,4,5,6 CCL_LOG_LEVEL=info CCL_ATL_SYNC_COLL=1"
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

echo -e "\nstart CCL test\n"
for coll in ${ccl_coll_list}
do
    cmd=`echo ${base_env} mpiexec -n 2 ${CCL_PATH}/examples/benchmark/benchmark -l ${coll} -i ${iters} -f ${msg_count} -t ${msg_count} -p 1 -n 1 -b sycl`
    echo "running: $cmd" | tee -a ${ccl_raw_log}
    eval $cmd 2>&1 | tee -a ${ccl_raw_log}
done

echo -e "\nstart MPI test\n"
for coll in ${mpi_coll_list}
do 
    cmd=`echo ${mpi_env} mpiexec -n 2 ${IMB_GPU_PATH} ${coll} -npmin 1024 -msglog ${msg_log_start}:${msg_log_end} -zero_size 0 -iter ${iters},2047`
    echo "running: $cmd" | tee -a ${mpi_raw_log}
    eval $cmd 2>&1 | tee -a ${mpi_raw_log}
done

echo -e "\nstart parsing\n"
echo -e "\nCCL results\n"
for coll in ${ccl_coll_list}
do
    timing=`cat ${ccl_raw_log} | grep -i "Benchmarking: ${coll}$ " -A 5 | tail -n 1 | awk '{ print $2 }'`
    echo "${coll}: ${timing}" | tee -a ${ccl_parsed_log}
done
echo -e "\nMPI results\n"
for coll in ${mpi_coll_list}
do
    timing=`cat ${mpi_raw_log} | grep -i "Benchmarking ${coll}$ " -A 4 | tail -n 1 | awk '{ print $4 }'`
    echo "${coll}: ${timing}" | tee -a ${mpi_parsed_log}
done

echo -e "\ncompleted\n"
