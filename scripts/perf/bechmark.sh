#! /bin/bash

WORK_DIR=
COMPILER_PATH=
CCL_PATH=
IMPI_PATH=
IMB_GPU_PATH=

cd ${WORK_DIR}

source ${COMPILER_PATH} intel64
source ${CCL_PATH} --ccl-configuration=cpu_gpu_dpcpp
source ${IMPI_PATH} --i_mpi_library_kind=release

echo kernel, agama, embargo version

base_env="I_MPI_ADJUST_ALLREDUCE=14 I_MPI_DEBUG=12 I_MPI_PIN_PROCESSOR_LIST=1,2"
base_env="${base_env} SYCL_DEVICE_FILTER=level_zero:*:0 SYCL_BE=PI_LEVEL_ZERO CreateMultipleSubDevices="
base_env="${base_env} CCL_WORKER_AFFINITY=3,4 CCL_ATL_SYNC_COLL=1"
base_env="${base_env} FI_PROVIDER=tcp"

mpi_env="${base_env} I_MPI_OFFLOAD=2 I_MPI_OFFLOAD_TOPOLIB=l0 I_MPI_OFFLOAD_CBWR=0"
mpi_env="${mpi_env} I_MPI_OFFLOAD_QUEUE_CACHE=1 I_MPI_OFFLOAD_LIST_CACHE=1"

coll_list="allgatherv allreduce alltoall bcast reduce reduce_scatter"
msg_size=2097152
msg_log=23
iters=100

ccl_log="${WORK_DIR}/ccl_perf.log"
mpi_log="${WORK_DIR}/mpi_perf.log"

for coll in $coll_list
do
	cmd="${base_env} mpiexec -n 2 ${CCL_PATH}/examples/benchmark/benchmark -l $coll -i ${iters} -f ${msg_size} -t ${msg_size} -p 1 -n 1 -b sycl 2>&1 | tee ${ccl_log}"
done

for coll in $coll_list
do 
	cmd="${mpi_env} mpiexec -n 2 ${IMB_GPU_PATH}/IMB-MPI1-GPU $coll -npmin 1024 -msglog ${msg_log}:${msg_log} -zero_size 0 -iter ${iters},2047 2>&1 | tee ${mpi_log}"
done

