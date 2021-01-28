#! /bin/bash

# for ATS 2T

WORK_DIR="/home/gta/ccl_mpi_perf"
COMPILER_PATH="/home/gta/oneapigold/compiler/latest"
CCL_PATH="${WORK_DIR}/mlsl2/build/_install"
IMPI_PATH="/home/gta/intel/impi_2021.2.0/"
IMB_GPU_PATH="/home/gta/ksenyako/IMB-MPI1-GPU"

CCL_URL="https://github.intel.com/ict/mlsl2.git"
CCL_BRANCH="master"
BUILD_CCL="0"

common_log="${WORK_DIR}/common.log"
ccl_log="${WORK_DIR}/ccl.log"
mpi_log="${WORK_DIR}/mpi.log"

mkdir -p ${WORK_DIR}
cd ${WORK_DIR}
rm ${common_log} ${ccl_log} ${mpi_log}

source ${COMPILER_PATH}/env/vars.sh intel64

if [ "${BUILD_CCL}" = "1" ]
then
	rm -r ./mlsl2
	git clone -b ${CCL_BRANCH} --single-branch ${CCL_URL}
	cd ./mlsl2
	mkdir build
	cd build
	cmake .. -DBUILD_UT=0 -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=dpcpp -DCOMPUTE_BACKEND=dpcpp
	make -j12 install
fi

source ${CCL_PATH}/env/setvars.sh --ccl-configuration=cpu_gpu_dpcpp
source ${IMPI_PATH}/env/vars.sh --i_mpi_library_kind=release

base_env="I_MPI_ADJUST_ALLREDUCE=14 I_MPI_DEBUG=12 I_MPI_PIN_PROCESSOR_LIST=1,2"
base_env="${base_env} SYCL_DEVICE_FILTER=level_zero:*:0 SYCL_BE=PI_LEVEL_ZERO CreateMultipleSubDevices="
base_env="${base_env} CCL_WORKER_AFFINITY=3,4 CCL_ATL_SYNC_COLL=1"
base_env="${base_env} FI_PROVIDER=tcp"

mpi_env="${base_env} I_MPI_OFFLOAD=2 I_MPI_OFFLOAD_TOPOLIB=l0 I_MPI_OFFLOAD_CBWR=0"
mpi_env="${mpi_env} I_MPI_OFFLOAD_QUEUE_CACHE=1 I_MPI_OFFLOAD_LIST_CACHE=1"

ccl_coll_list="allgatherv allreduce alltoall bcast reduce reduce_scatter"
mpi_coll_list="allgatherv allreduce alltoall bcast reduce_scatter" # skip reduce since it takes too much time

msg_count=2097152
msg_log=23
iters=100

lscpu | tee -a ${common_log}
uname -a | tee -a ${common_log}
dpkg --list | grep level-zero | tee -a tee ${common_log}
mpiexec -V | tee -a ${common_log}

echo "start CCL test"
for coll in ${ccl_coll_list}
do
    cmd=`echo ${base_env} mpiexec -n 2 ${CCL_PATH}/examples/benchmark/benchmark -l ${coll} -i ${iters} -f ${msg_count} -t ${msg_count} -p 1 -n 1 -b sycl`
    echo "running: $cmd" | tee -a ${ccl_log}
    eval $cmd 2>&1 | tee -a ${ccl_log}
done

echo "start MPI test"
for coll in ${mpi_coll_list}
do 
    cmd=`echo ${mpi_env} mpiexec -n 2 ${IMB_GPU_PATH} ${coll} -npmin 1024 -msglog ${msg_log}:${msg_log} -zero_size 0 -iter ${iters},2047`
    echo "running: $cmd" | tee -a ${mpi_log}
    eval $cmd 2>&1 | tee -a ${mpi_log}
done

echo "completed"

# parse results
