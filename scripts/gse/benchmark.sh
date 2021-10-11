#!/bin/bash

set_env()
{
	export I_MPI_ROOT=./mpi_lib/
	export CCL_ROOT=./ccl/
	export PATH=$I_MPI_ROOT/bin:$PATH
	export LD_LIBRARY_PATH=$I_MPI_ROOT/libfabric/lib:$I_MPI_ROOT/release:./compiler_lib:./compiler_lib/x64:./compiler_lib/intel64_lin:./compiler_lib/lib:$CCL_ROOT/lib/cpu_gpu_dpcpp::$LD_LIBRARY_PATH
	export FI_PROVIDER_PATH=$I_MPI_ROOT/libfabric/lib/prov
	
	env
	ldd ./benchmark
}

set_env
case $1 in
"level_zero" )
	echo "MODE: " $1
	CCL_ATL_MPI_LIB_TYPE=impi SYCL_PI_TRACE=1 SYCL_DEVICE_FILTER=level_zero:*:0 mpiexec -n 2 ./benchmark -b sycl -y $2 -n 1 -a gpu -l allreduce
	shift
	;;
"opencl" )
	echo "MODE: " $1
	CCL_ATL_MPI_LIB_TYPE=impi SYCL_PI_TRACE=1 SYCL_DEVICE_FILTER=opencl:*:0 mpiexec -n 2 ./benchmark -b sycl -y $2 -n 1 -a gpu -l allreduce
	shift
	;;
*)
	echo "WARNING: testing not started, please run ./benchmark.sh level_zero|opencl"
	exit 0
	shift
	;;
esac
