#!/bin/bash

CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo "${2}" 1>&2
        exit $1
    fi
}

# parse_parameters() {
#     while [ $# -gt 0 ]
#     do
#         key="$1"
#         case $key in
#             -c|--comparison)
#             ${COMPARISON}="YES"; shift;
#             ;;
#             -n)
#             export BENCH_CONF_N="$2"; shift; shift;
#             ;;
#             -ppn)
#             export BENCH_CONF_PPN="$2"; shift; shift;
#             ;;
#             DG1)
#             export SYSTEM_NAME="DG1"; shift;
#             ;;
#             ATS)
#             export SYSTEM_NAME="ATS"; shift;
#             ;;
#             -*|--*)
#             echo "  ERROR: Unsupported flag $key"; exit 1
#             ;;
#         esac
#     done
# }

set_default_values()
{
    PERF_WORK_DIR="/panfs/users/dsazanov/performance"
    SYSTEM_NAME="DG1"
    COMPARISON=""
    BENCH_CONF_N="2"
    BENCH_CONF_PPN="1"
}

#GET HOSTFILE
get_host_list() {
    HOSTFILE="$(ls ${PERF_WORK_DIR}/jobinfo -t | head -1)"
    echo "HOSTFILE = ${HOSTFILE}"
}

#GET QUEUE
reserve_nodes() {
    cd ${PERF_WORK_DIR}/jobinfo
    bsub -P R -p dgq -N 2 -C 'clx8260dg1' -t 10 -Is /bin/bash
    CheckCommandExitCode $? "  ERROR: Getting nodes error"
}

#SOURCE MPI & COMPILER 
source_environment() {
    echo "MESSAGE: Source MPi & compiler"
    #source /opt/crtdc/dg1/2020-09-24/VARS.sh
    source ${PERF_WORK_DIR}/oneapi/compiler/_install/setvars.sh
    source ${PERF_WORK_DIR}/oneapi/mpi_oneapi/_install/mpi/latest/env/vars.sh release_mt
}

build() {
    mkdir build
    cd build
    cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=dpcpp  -DCOMPUTE_RUNTIME=dpcpp
    CheckCommandExitCode $? "  ERROR: cmake .. error"
    make -j8 install
    CheckCommandExitCode $? "  ERROR: make error"
}

build_ccl() {
    if [ -z ${COMPARISON} ]
    then
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
    echo "START BENCH"
}

# unpacking_ccl() {
#     if [ -z ${COMPARISON} ]
#     then
#         PACKAGE_NAME=$(ls ${PERF_WORK_DIR}/ccl -t | head -1)
#         cd ${PERF_WORK_DIR}/ccl
#         tar xvzf ${PERF_WORK_DIR}/ccl/${PACKAGE_NAME}
#         export PACKAGE_NAME=$(ls ${PERF_WORK_DIR}/ccl -t | tail -1)
#         unzip -P accept ${PERF_WORK_DIR}/ccl/${PACKAGE_NAME}/package.zip 
#         ${PERF_WORK_DIR}/ccl/install.sh -d ${PERF_WORK_DIR}/ccl/_install -s
#         mkdir ${PERF_WORK_DIR}/ccl/_install/build
#         cd ${PERF_WORK_DIR}/ccl/_install/build
#         cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=dpcpp  -DCOMPUTE_RUNTIME=dpcpp
#         CheckCommandExitCode $? "  ERROR: cmake .. error"
#         make -j8 install
#         CheckCommandExitCode $? "  ERROR: make error"
#         echo "MESSAGE: Install ccl package - ${PACKAGE_NAME}: SUCCESS"
#     else
#         echo "MESSAGE: comparison ccl"
#         PACKAGE_NAME=$(ls ${PERF_WORK_DIR}/ccl_c1 -t | head -1)
#         echo "   INFO: FIRST_PACKAGE_NAME = ${PACKAGE_NAME}"
#         echo " ccl_c1 - ${PACKAGE_NAME}" >> ${PERF_WORK_DIR}/logs/comparison/info.txt
#         cd ${PERF_WORK_DIR}/ccl_c1
#         tar xvzf ${PERF_WORK_DIR}/ccl_c1/${PACKAGE_NAME}
#         export PACKAGE_NAME=$(ls ${PERF_WORK_DIR}/ccl -t | tail -1)
#         unzip -P accept ${PERF_WORK_DIR}/ccl_c1/${PACKAGE_NAME}/package.zip 
#         ${PERF_WORK_DIR}/ccl/${PACKAGE_NAME}/install.sh -d ${PERF_WORK_DIR}/ccl_c1/_install -s
#         mkdir ${PERF_WORK_DIR}/ccl_c1/_install/build
#         cd ${PERF_WORK_DIR}/ccl_c1/_install/build
#         cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=dpcpp  -DCOMPUTE_RUNTIME=dpcpp
#         CheckCommandExitCode $? "  ERROR: cmake .. error"
#         make -j8 install
#         CheckCommandExitCode $? "  ERROR: make error"
#         echo "MESSAGE: Install first package - ${PACKAGE_NAME}: SUCCESS"

#         PACKAGE_NAME=$(ls ${PERF_WORK_DIR}/ccl_c2 -t | head -1)
#         echo "   INFO: SECOND_PACKAGE_NAME = ${PACKAGE_NAME}"
#         echo " ccl_c2 - ${PACKAGE_NAME}" > ${PERF_WORK_DIR}/logs/comparison/info.txt
#         cd ${PERF_WORK_DIR}/ccl_2
#         tar xvzf ${PERF_WORK_DIR}/ccl_c2/${PACKAGE_NAME}
#         export PACKAGE_NAME=$(ls ${PERF_WORK_DIR}/ccl -t | tail -1)
#         unzip -P accept ${PERF_WORK_DIR}/ccl_c2/${PACKAGE_NAME}/package.zip 
#         ${PERF_WORK_DIR}/ccl/${PACKAGE_NAME}/install.sh -d ${PERF_WORK_DIR}/ccl_c2/_install -s
#         mkdir ${PERF_WORK_DIR}/ccl_c2/_install/build
#         cd ${PERF_WORK_DIR}/ccl_c2/_install/build
#         cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=dpcpp  -DCOMPUTE_RUNTIME=dpcpp
#         CheckCommandExitCode $? "  ERROR: cmake .. error"
#         make -j8 install
#         CheckCommandExitCode $? "  ERROR: make error"
#         echo "MESSAGE: Install second package - ${PACKAGE_NAME}: SUCCESS"

#     fi
#     echo "START BENCHMARK!"
# }

start_benchmark() {
    if [[ ${SYSTEM_NAME} == "DG1" ]]
    then
        # BENCH FOR DG1
        if [ -z ${COMPARISON} ]
        then
            source ${PERF_WORK_DIR}/ccl/build/_install/env/setvars.sh
            for i in {1..5}
	        do
                CCL_WORKER_AFFINITY=2,3,4,5 I_MPI_PIN_PROCESSOR_LIST=1 SYCL_BE=PI_OPENCL mpiexec -n ${BENCH_CONF_N} -ppn ${BENCH_CONF_PPN} -hostfile ${PERF_WORK_DIR}/jobinfo/${HOSTFILE} ${PERF_WORK_DIR}/ccl/build/_install/examples/benchmark/benchmark -l allreduce -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m buf > ${PERF_WORK_DIR}/logs/single/DG1_OCL/Allreduce_${BENCH_CONF_N}_${BENCH_CONF_PPN}_$i.log
                CheckCommandExitCode $? "DG1 BENCHMARK ERROR: OCL, allreduce, ccl"
                CL_WORKER_AFFINITY=2,3,4,5 I_MPI_PIN_PROCESSOR_LIST=1 SYCL_BE=PI_LEVEL0 mpiexec -n ${BENCH_CONF_N} -ppn ${BENCH_CONF_PPN} -hostfile ${PERF_WORK_DIR}/jobinfo/${HOSTFILE} ${PERF_WORK_DIR}/ccl/build/_install/examples/benchmark/benchmark -l allreduce -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m buf > ${PERF_WORK_DIR}/logs/single/DG1_L0/Allreduce_${BENCH_CONF_N}_${BENCH_CONF_PPN}_$i.log
                CheckCommandExitCode $? "DG1 BENCHMARK ERROR: L0, allreduce, ccl"
	            echo " $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i"
	        done
        else
            source ${PERF_WORK_DIR}/ccl_c1/build/_install/env/setvars.sh
            for i in {1..5}
	        do
                CCL_WORKER_AFFINITY=2,3,4,5 I_MPI_PIN_PROCESSOR_LIST=1 SYCL_BE=PI_OPENCL mpiexec -n ${BENCH_CONF_N} -ppn ${BENCH_CONF_PPN} -hostfile ${PERF_WORK_DIR}/jobinfo/${HOSTFILE} ${PERF_WORK_DIR}/ccl_c1/build/_install/examples/benchmark/benchmark -l allreduce -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m buf > ${PERF_WORK_DIR}/logs/comparison/ccl_c1/DG1_OCL/Allreduce_${BENCH_CONF_N}_${BENCH_CONF_PPN}_$i.log
                CheckCommandExitCode $? "BENCHMARK ERROR: OCL, allreduce, ccl_c1"
                CL_WORKER_AFFINITY=2,3,4,5 I_MPI_PIN_PROCESSOR_LIST=1 SYCL_BE=PI_LEVEL0 mpiexec -n ${BENCH_CONF_N} -ppn ${BENCH_CONF_PPN} -hostfile ${PERF_WORK_DIR}/jobinfo/${HOSTFILE} ${PERF_WORK_DIR}/ccl_c1/build/_install/examples/benchmark/benchmark -l allreduce -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m buf > ${PERF_WORK_DIR}/logs/comparison/ccl_c1/DG1_L0/Allreduce_${BENCH_CONF_N}_${BENCH_CONF_PPN}_$i.log
                CheckCommandExitCode $? "BENCHMARK ERROR: L0, allreduce ccl_c1"
	            echo " $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i"
	        done

            source ${PERF_WORK_DIR}/ccl_c2/build/_install/env/setvars.sh
            for i in {1..5}
	        do
                CCL_WORKER_AFFINITY=2,3,4,5 I_MPI_PIN_PROCESSOR_LIST=1 SYCL_BE=PI_OPENCL mpiexec -n ${BENCH_CONF_N} -ppn ${BENCH_CONF_PPN} -hostfile ${PERF_WORK_DIR}/jobinfo/${HOSTFILE} ${PERF_WORK_DIR}/ccl_c2/build/_install/examples/benchmark/benchmark -l allreduce -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m buf > ${PERF_WORK_DIR}/logs/comparison/ccl_c2/DG1_OCL/Allreduce_${BENCH_CONF_N}_${BENCH_CONF_PPN}_$i.log
                CheckCommandExitCode $? "BENCHMARK ERROR: OCL, allreduce, ccl_c1"
                CL_WORKER_AFFINITY=2,3,4,5 I_MPI_PIN_PROCESSOR_LIST=1 SYCL_BE=PI_LEVEL0 mpiexec -n ${BENCH_CONF_N} -ppn ${BENCH_CONF_PPN} -hostfile ${PERF_WORK_DIR}/jobinfo/${HOSTFILE} ${PERF_WORK_DIR}/ccl_c2/build/_install/examples/benchmark/benchmark -l allreduce -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m buf > ${PERF_WORK_DIR}/logs/comparison/ccl_c2/DG1_L0/Allreduce_${BENCH_CONF_N}_${BENCH_CONF_PPN}_$i.log
                CheckCommandExitCode $? "BENCHMARK ERROR: L0, allreduce ccl_c1"
	            echo " $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i"
	        done
        fi

    else
        # BENCH FOR ATS
        if [ -z ${COMPARISON} ]
        then
            source ${PERF_WORK_DIR}/ccl/build/_install/env/setvars.sh
            for i in {1..5}
	        do
                CCL_ATL_SYNC_COLL=1 I_MPI_ADJUST_ALLREDUCE=14 CreateMultipleSubDevices= SYCL_BE=PI_LEVEL_ZERO I_MPI_PIN_PROCESSOR_LIST=1,2 CCL_WORKER_AFFINITY=3,4 mpiexec -n ${BENCH_CONF_N} ${PERF_WORK_DIR}/ccl/build/_install/examples/benchmark/benchmark -l allreduce -i 100 -f 2097152 -t 2097152 -p 1 -b host -m usm -u device > ${PERF_WORK_DIR}/logs/single/ATS_OCL/Allreduce_${BENCH_CONF_N}_1_$i.log
                CheckCommandExitCode $? "ATS BENCHMARK ERROR: OCL, allreduce, ccl"
                CCL_ATL_SYNC_COLL=1 I_MPI_ADJUST_ALLREDUCE=14 CreateMultipleSubDevices= SYCL_BE=PI_OPENCL I_MPI_PIN_PROCESSOR_LIST=1,2 CCL_WORKER_AFFINITY=3,4 mpiexec -n ${BENCH_CONF_N} ${PERF_WORK_DIR}/ccl/build/examples/benchmark/benchmark -l allreduce -i 100 -f 2097152 -t 2097152 -p 1 -b host -m usm -u device > ${PERF_WORK_DIR}/logs/single/ATS_L0/Allreduce_${BENCH_CONF_N}_1_$i.log
                CheckCommandExitCode $? "ATS BENCHMARK ERROR: L0, allreduce, ccl"
	            echo " $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i"
	        done
        else
            source ${PERF_WORK_DIR}/ccl_c1/build/_install/env/setvars.sh
            for i in {1..5}
	        do
                CCL_ATL_SYNC_COLL=1 I_MPI_ADJUST_ALLREDUCE=14 CreateMultipleSubDevices= SYCL_BE=PI_LEVEL_ZERO I_MPI_PIN_PROCESSOR_LIST=1,2 CCL_WORKER_AFFINITY=3,4 mpiexec -n ${BENCH_CONF_N} ${PERF_WORK_DIR}/ccl_c1/build/_install/examples/benchmark/benchmark -l allreduce -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m usm -u device > ${PERF_WORK_DIR}/logs/comparison/ccl_c1/ATS_OCL/Allreduce_${BENCH_CONF_N}_1_$i.log
                CheckCommandExitCode $? "BENCHMARK ERROR: OCL, allreduce, ccl_c1"
                CCL_ATL_SYNC_COLL=1 I_MPI_ADJUST_ALLREDUCE=14 CreateMultipleSubDevices= SYCL_BE=PI_OPENCL I_MPI_PIN_PROCESSOR_LIST=1,2 CCL_WORKER_AFFINITY=3,4 mpiexec -n ${BENCH_CONF_N} ${PERF_WORK_DIR}/ccl_c1/build/_install/examples/benchmark/benchmark -l allreduce -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m usm -u device > ${PERF_WORK_DIR}/logs/comparison/ccl_c1/ATS_L0/Allreduce_${BENCH_CONF_N}_1_$i.log
                CheckCommandExitCode $? "BENCHMARK ERROR: L0, allreduce, ccl_c1"
	            echo " $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i"
	        done

            source ${PERF_WORK_DIR}/ccl_c2/build/_install/env/setvars.sh
            for i in {1..5}
	        do
                CCL_ATL_SYNC_COLL=1 I_MPI_ADJUST_ALLREDUCE=14 CreateMultipleSubDevices= SYCL_BE=PI_LEVEL_ZERO I_MPI_PIN_PROCESSOR_LIST=1,2 CCL_WORKER_AFFINITY=3,4 mpiexec -n ${BENCH_CONF_N} ${PERF_WORK_DIR}/ccl_c2/build/_install/examples/benchmark/benchmark -l allreduce -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m usm -u device > ${PERF_WORK_DIR}/logs/comparison/ccl_c2/ATS_OCL/Allreduce_${BENCH_CONF_N}_1_$i.log
                CheckCommandExitCode $? "BENCHMARK ERROR: OCL, allreduce, ccl_c2"
                CCL_ATL_SYNC_COLL=1 I_MPI_ADJUST_ALLREDUCE=14 CreateMultipleSubDevices= SYCL_BE=PI_OPENCL I_MPI_PIN_PROCESSOR_LIST=1,2 CCL_WORKER_AFFINITY=3,4 mpiexec -n ${BENCH_CONF_N} ${PERF_WORK_DIR}/ccl_c2/build/_install/examples/benchmark/benchmark -l allreduce -i 100 -f 2097152 -t 2097152 -p 1 -b sycl -m usm -u device > ${PERF_WORK_DIR}/logs/comparison/ccl_c2/ATS_L0/Allreduce_${BENCH_CONF_N}_1_$i.log
                CheckCommandExitCode $? "BENCHMARK ERROR: L0, allreduce, ccl_c2"
	            echo " $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i ITER $i"
	        done
        fi
    fi
}

set_default_values
#parse_parameters

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
            export SYSTEM_NAME="ATS"; export PERF_WORK_DIR="/home/gta/dsazanov/performance"; shift;
            ;;
            -*|--*)
            echo "  ERROR: Unsupported flag $key"; exit 1
            ;;
        esac
    done

if [[ ${SYSTEM_NAME} == "DG1" ]]
then
    #reserve_nodes
    get_host_list
fi

source_environment
build_ccl
start_benchmark
