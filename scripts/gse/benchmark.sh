#!/bin/bash

set_env()
{
	export I_MPI_ROOT=./mpi_lib/
	export CCL_ROOT=./ccl/
	export PATH=$I_MPI_ROOT/bin:$PATH
	export LD_LIBRARY_PATH=$I_MPI_ROOT/libfabric/lib:$I_MPI_ROOT/release:./compiler_lib:./compiler_lib/x64:./compiler_lib/intel64_lin:./compiler_lib/lib:$CCL_ROOT/lib/cpu_gpu_dpcpp::$LD_LIBRARY_PATH
	export FI_PROVIDER_PATH=$I_MPI_ROOT/libfabric/lib/prov

	# CCL
	export CCL_LOG_LEVEL=info
	export CCL_ATL_MPI_LIB_TYPE=impi

	# SYCL
	export SYCL_PI_TRACE=1
	
	env
	ldd ./benchmark
}

set_env
mode=${1}
n=${2}
msg_size=${3}
sycl_root_dev=${4}

echo "[DEBUG]: MODE          = ${mode}"
echo "[DEBUG]: N             = ${n}"
echo "[DEBUG]: MSG_SIZE      = ${msg_size}"
echo "[DEBUG]: SYCL_ROOT_DEV = ${sycl_root_dev}"

case ${mode} in
"level_zero" )
	SYCL_DEVICE_FILTER=level_zero mpiexec -n ${n} ./benchmark -c all -g ${sycl_root_dev} -d int32 -b sycl -y ${msg_size} -a gpu -l all
	shift
	;;
"opencl" )
	SYCL_DEVICE_FILTER=opencl mpiexec -n ${n} ./benchmark -c all -g ${sycl_root_dev} -d int32 -b sycl -y ${msg_size} -a gpu -l allreduce
	shift
	;;
*)
	echo "WARNING: testing not started, please run ./benchmark.sh <level_zero|opencl> <n> <msg_size> <sycl_root_dev>"
	exit 0
	;;
esac
