#!/bin/bash

CheckCommandExitCode() 
{
    if [ $1 -ne 0 ]; then
        echo "${2}" 1>&2
        exit $1
    fi
}

echo_log()
{
    msg=$1
    date=`date`
    echo "[$date] > $msg"
}

set_default_values()
{
    PERF_WORK_DIR="/home/gta/dsazanov/performance"
    SYSTEM_NAME="ATS"
    COMPARISON=""
    BENCH_CONF_N="2"
    BENCH_CONF_PPN="1"
    CCL_BUILD="1"
    SYSTEM_LOGIN="gta"
    START_BENCHMARK_TYPE="default"
    ALL_COLL=(alltoall alltoallv bcast reduce reduce_scatter allgatherv allreduce )
    ALL_COLL_RN=(Alltoall Alltoallv Bcast Reduce Reduce_scatter Allgatherv Allreduce)
}

source_environment() 
{
    echo_log "MESSAGE: Source MPi & compiler"
    source ${PERF_WORK_DIR}/oneapi/compiler/_install/setvars.sh
    source ${PERF_WORK_DIR}/oneapi/mpi_oneapi/_install/mpi/latest/env/vars.sh
}

build() 
{
    echo_log "MESSAGE: Build oneCCL"
    mkdir build
    cd build
    cmake ..  -DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=dpcpp  -DCOMPUTE_BACKEND=dpcpp_level_zero
    CheckCommandExitCode $? "  ERROR: Build oneCCL - cmake .. error"
    make -j8 install
    CheckCommandExitCode $? "  ERROR: Build oneCCL - make error"
    echo_log "MESSAGE: Build oneCCL: SUCCESS"
}

build_ccl() 
{
    if [ -z ${COMPARISON} ]; then
        cd ${PERF_WORK_DIR}/ccl
        tar xvzf ccl_src.tgz
        build
    else
        echo "MESSAGE: comparison ccl"
        cd ${PERF_WORK_DIR}/ccl_c1
        tar xvzf ccl_src.tgz
        build

        cd ${PERF_WORK_DIR}/ccl_c2
        tar xvzf ccl_src.tgz
        build
    fi
}

parse_parameters()
{
    while [ $# -gt 0 ]
    do
        key="$1"
        case $key in
            -c|--comparison)
            ${COMPARISON}="YES"; shift;
            ;;
            -n)
            export BENCH_CONF_N="$2"; shift; shift;
            ;;
            -ppn)
            export BENCH_CONF_PPN="$2"; shift; shift;
            ;;
            DG1)
            export SYSTEM_NAME="DG1"; export PERF_WORK_DIR="/panfs/users/dsazanov/performance"; shift;
            ;;
            ATS)
            export SYSTEM_NAME="ATS"; shift;
            ;;
            -sl)
            export SYSTEM_LOGIN="$2"; export PERF_WORK_DIR="/home/${SYSTEM_LOGIN}/dsazanov/performance"; shift; shift;
            ;;
            -UIC)
            export CCL_BUILD="$2"; shift; shift;
            ;;
            -bt)
            export START_BENCHMARK_TYPE="$2"; shift; shift;
            ;;
            -*|--*)
            echo_log "  ERROR: Unsupported flag $key"; exit 1
            ;;
        esac
    done
}

clear_log_dir()
{
    if [ -z ${COMPARISON} ]; then
        rm -f ${PERF_WORK_DIR}/logs/single/ATS_OCL/*
        rm -f ${PERF_WORK_DIR}/logs/single/ATS_L0/*
    else
        rm -f ${PERF_WORK_DIR}/logs/comparison/ccl_c1/ATS_OCL/*
        rm -f ${PERF_WORK_DIR}/logs/comparison/ccl_c1/ATS_L0/*
        rm -f ${PERF_WORK_DIR}/logs/comparison/ccl_c2/ATS_OCL/*
        rm -f ${PERF_WORK_DIR}/logs/comparison/ccl_c2/ATS_L0/*
    fi
}

#==============================================================================
#                              Benchmark launches
#==============================================================================

start_benchmark_default() 
{

for cl in {0..6}; do

    if [ -z ${COMPARISON} ]; then
        source ${PERF_WORK_DIR}/ccl/build/_install/env/setvars.sh
        for i in {1..5}; do
            echo_log "MESSAGE: Iteration number = $i | -l ${ALL_COLL[$cl]}"
            CCL_ATL_SYNC_COLL=1 I_MPI_ADJUST_ALLREDUCE=14 CreateMultipleSubDevices= SYCL_BE=PI_LEVEL_ZERO I_MPI_PIN_PROCESSOR_LIST=1,2 CCL_WORKER_AFFINITY=3,4 mpiexec -n ${BENCH_CONF_N} ${PERF_WORK_DIR}/ccl/build/_install/examples/benchmark/benchmark -l ${ALL_COLL[$cl]} -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m usm -u device > ${PERF_WORK_DIR}/logs/single/ATS_L0/${ALL_COLL_RN[$cl]}-${BENCH_CONF_N}-1-$i.log
            CheckCommandExitCode $? "ATS BENCHMARK ERROR: OCL, ${ALL_COLL[$cl]} ccl"
            CCL_ATL_SYNC_COLL=1 I_MPI_ADJUST_ALLREDUCE=14 CreateMultipleSubDevices= SYCL_BE=PI_OPENCL I_MPI_PIN_PROCESSOR_LIST=1,2 CCL_WORKER_AFFINITY=3,4 mpiexec -n ${BENCH_CONF_N} ${PERF_WORK_DIR}/ccl/build/examples/benchmark/benchmark -l ${ALL_COLL[$cl]} -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m usm -u device > ${PERF_WORK_DIR}/logs/single/ATS_OCL/${ALL_COLL_RN[$cl]}-${BENCH_CONF_N}-1-$i.log
            CheckCommandExitCode $? "ATS BENCHMARK ERROR: L0, ${ALL_COLL[$cl]} ccl"
	    done
    else
        source ${PERF_WORK_DIR}/ccl_c1/build/_install/env/setvars.sh
        for i in {1..5}; do
            echo_log "MESSAGE: Iteration number = $i | -l ${ALL_COLL[$cl]}"
            CCL_ATL_SYNC_COLL=1 I_MPI_ADJUST_ALLREDUCE=14 CreateMultipleSubDevices= SYCL_BE=PI_LEVEL_ZERO I_MPI_PIN_PROCESSOR_LIST=1,2 CCL_WORKER_AFFINITY=3,4 mpiexec -n ${BENCH_CONF_N} ${PERF_WORK_DIR}/ccl_c1/build/_install/examples/benchmark/benchmark -l ${ALL_COLL[$cl]} -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m usm -u device > ${PERF_WORK_DIR}/logs/comparison/ccl_c1/ATS_L0/${ALL_COLL_RN[$cl]}_${BENCH_CONF_N}_1_$i.log
            CheckCommandExitCode $? "BENCHMARK ERROR: OCL, ${ALL_COLL[$cl]} ccl_c1"
            CCL_ATL_SYNC_COLL=1 I_MPI_ADJUST_ALLREDUCE=14 CreateMultipleSubDevices= SYCL_BE=PI_OPENCL I_MPI_PIN_PROCESSOR_LIST=1,2 CCL_WORKER_AFFINITY=3,4 mpiexec -n ${BENCH_CONF_N} ${PERF_WORK_DIR}/ccl_c1/build/_install/examples/benchmark/benchmark -l ${ALL_COLL[$cl]} -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m usm -u device > ${PERF_WORK_DIR}/logs/comparison/ccl_c1/ATS_OCL/${ALL_COLL_RN[$cl]}_${BENCH_CONF_N}_1_$i.log
            CheckCommandExitCode $? "BENCHMARK ERROR: L0, ${ALL_COLL[$cl]} ccl_c1"
	    done

        source ${PERF_WORK_DIR}/ccl_c2/build/_install/env/setvars.sh
        for i in {1..5}; do
            echo_log "MESSAGE: Iteration number = $i | -l ${ALL_COLL[$cl]}"
            CCL_ATL_SYNC_COLL=1 I_MPI_ADJUST_ALLREDUCE=14 CreateMultipleSubDevices= SYCL_BE=PI_LEVEL_ZERO I_MPI_PIN_PROCESSOR_LIST=1,2 CCL_WORKER_AFFINITY=3,4 mpiexec -n ${BENCH_CONF_N} ${PERF_WORK_DIR}/ccl_c2/build/_install/examples/benchmark/benchmark -l ${ALL_COLL[$cl]} -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m usm -u device > ${PERF_WORK_DIR}/logs/comparison/ccl_c2/ATS_L0/${ALL_COLL_RN[$cl]}_${BENCH_CONF_N}_1_$i.log
            CheckCommandExitCode $? "BENCHMARK ERROR: OCL, ${ALL_COLL[$cl]}, ccl_c2"
            CCL_ATL_SYNC_COLL=1 I_MPI_ADJUST_ALLREDUCE=14 CreateMultipleSubDevices= SYCL_BE=PI_OPENCL I_MPI_PIN_PROCESSOR_LIST=1,2 CCL_WORKER_AFFINITY=3,4 mpiexec -n ${BENCH_CONF_N} ${PERF_WORK_DIR}/ccl_c2/build/_install/examples/benchmark/benchmark -l ${ALL_COLL[$cl]} -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m usm -u device > ${PERF_WORK_DIR}/logs/comparison/ccl_c2/ATS_OCL/${ALL_COLL_RN[$cl]}_${BENCH_CONF_N}_1_$i.log
            CheckCommandExitCode $? "BENCHMARK ERROR: L0, ${ALL_COLL[$cl]} ccl_c2"
	    done
    fi

done
}

start_benchmark_embargo395() 
{

    EMB_COLL=(allgatherv alltoall bcast reduce reduce_scatter allreduce)
    EMB_COLL_RN=(Allgatherv Alltoall Bcast Reduce Reduce_scatter Allreduce)
    source ${PERF_WORK_DIR}/ccl/build/_install/env/setvars.sh

    for cl in {0..5}; do

        for i in {1..5}; do
            echo_log "MESSAGE: Iteration number = $i | -l ${EMB_COLL[$cl]}"
            CCL_ATL_SYNC_COLL=1 I_MPI_ADJUST_ALLREDUCE=14 CreateMultipleSubDevices= SYCL_DEVICE_FILTER=level_zero:*:0  I_MPI_PIN_PROCESSOR_LIST=1,2 CCL_WORKER_AFFINITY=3,4 mpiexec -n ${BENCH_CONF_N} ${PERF_WORK_DIR}/ccl/build/_install/examples/benchmark/benchmark -l ${EMB_COLL[$cl]} -i 100 -f 2097152 -t 2097152 -n 1 -b sycl > ${PERF_WORK_DIR}/logs/single/ATS_L0/${EMB_COLL_RN[$cl]}_${BENCH_CONF_N}_1_$i.log
            CheckCommandExitCode $? "ATS BENCHMARK ERROR: ${EMB_COLL[$cl]}"
	    done

    done
}

start_benchmark_embargo395_gold() 
{

    EMB_COLL=(allgatherv allreduce alltoall bcast reduce reduce_scatter)
    EMB_COLL_RN=(Allgatherv Allreduce Alltoall Bcast Reduce Reduce_scatter)
    source ${PERF_WORK_DIR}/ccl_gold/l_ccl_release__2021.1.0_RC_ww45.20201105.212942/env/vars.sh

    for cl in {0..5}; do

        for i in {1..5}; do
            echo_log "MESSAGE: Iteration number = $i | -l ${EMB_COLL[$cl]}"
            CCL_ATL_SYNC_COLL=1 I_MPI_ADJUST_ALLREDUCE=14 CreateMultipleSubDevices= SYCL_DEVICE_FILTER=level_zero:*:0 SYCL_BE=PI_LEVEL_ZERO  I_MPI_PIN_PROCESSOR_LIST=1,2 CCL_WORKER_AFFINITY=3,4 mpiexec -n ${BENCH_CONF_N} ${PERF_WORK_DIR}/ccl/build/_install/examples/benchmark/benchmark --coll ${EMB_COLL[$cl]} -i 100 -f 2097152 -t 2097152 -p 1 -b sycl > ${PERF_WORK_DIR}/logs/single/ATS_L0/${EMB_COLL_RN[$cl]}_${BENCH_CONF_N}_1_$i.log
            CheckCommandExitCode $? "ATS BENCHMARK ERROR: ${EMB_COLL[$cl]}"
	    done

    done
}

#==============================================================================
#                                     MAIN
#==============================================================================

set_default_values
parse_parameters $@
source_environment

if [[ ${CCL_BUILD} == "1" ]]; then
    build_ccl
fi

clear_log_dir

if [[ ${START_BENCHMARK_TYPE} == "default" ]]; then
    start_benchmark_default
fi

if [[ ${START_BENCHMARK_TYPE} == "embargo395" ]]; then
    start_benchmark_embargo395
fi

if [[ ${START_BENCHMARK_TYPE} == "embargo395_gold" ]]; then
    start_benchmark_embargo395_gold
fi
