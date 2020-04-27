#!/bin/bash

touch main_ouput.txt
exec | tee ./main_ouput.txt

function create_work_dir(){
    if [ -z ${EXAMPLE_WORK_DIR} ]
    then
        SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
        export EXAMPLE_WORK_DIR=${SCRIPT_DIR}/build
        rm -r ${EXAMPLE_WORK_DIR}
        mkdir -p ${EXAMPLE_WORK_DIR}
    fi
    echo "EXAMPLE_WORK_DIR =" ${EXAMPLE_WORK_DIR}
}

CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo "ERROR: ${2}" 1>&2
        exit $1
    fi
}

declare -i total_fails=0
declare -i total_skipped=0

function check_test(){
    test_log=$1
    test_file=$2
    test_passed=`grep -E -c -i 'PASSED' ${test_log}`
    if [ $test_file != "communicator" ] && [ $test_file != "datatype" ];
    then
        test_failed=`grep -E -c -i 'error|Aborted|failed|^BAD$|KILLED|^fault$|cl::sycl::runtime_error|terminate' ${test_log}`
    else
        test_failed=`grep -E -c -i 'Aborted|failed|^BAD$|KILLED|^fault$|cl::sycl::runtime_error|terminate' ${test_log}`
    fi
    test_skipped=`grep -E -c -i 'unavailable|skipped' ${test_log}`
    if ([ ${test_passed} -eq 0 ] || [ ${test_skipped} -eq 0 ]) && [ ${test_failed} -ne 0 ]
    then
        echo "Error: example $test_file testing failed"
        echo "See log ${test_log} for details"
        total_fails=${total_fails}+1
    fi
    if [ ${test_skipped} -ne 0 ]
    then
        echo "Test $test_file has been skipped."
        echo "See log ${test_log} for details"
        total_skipped=${total_skipped}+1
    fi
}

export FI_PROVIDER=tcp

check_clang()
{
    which clang++
    COMPILER_INSTALL_CHECK=$?
    if [ "$COMPILER_INSTALL_CHECK" != "0" ]
    then
        echo "Error: Need to source clang++ compiler"
        exit 1
    fi
}
which mpiexec
MPI_INSTALL_CHECK=$?
if [ "$MPI_INSTALL_CHECK" != "0" ]
then
    echo "Error: IMPI wasn't found"
    exit 1
fi

MPI_INSTALL_CHECK_2=`echo $I_MPI_ROOT`
if [ "$MPI_INSTALL_CHECK_2" == "" ]
then
    echo "Error: I_MPI_ROOT wasn't found"
    exit 1
fi

CCL_INSTALL_CHECK=`echo $CCL_ROOT`
if [ "$CCL_INSTALL_CHECK" == "" ]
then
    echo "Error: CCL_ROOT wasn't found"
    exit 1
fi
run_benchmark()
{
    echo "================ENVIRONMENT=================="
    local ccl_extra_env=$1
    echo "ccl_extra_env: " $ccl_extra_env
    local dir_name=$2
    echo "dir_name: " $dir_name
    local transport=$3
    echo "transport: "$transport
    local example=$4
    echo "example: "$example
    local backend=$5
    echo "backend: "$backend
    local loop=$6
    echo "loop: "$loop
    local coll=$7
    echo "coll: " $coll
    echo "================ENVIRONMENT=================="
    test_log="$EXAMPLE_WORK_DIR/$dir_name/run_${transport}_${example}_${backend}_${loop}_output.log"
	if [ `echo $ccl_extra_env | grep -c CCL_LOG_LEVEL` -ne 1 ]
	then
		eval `echo $ccl_extra_env mpiexec.hydra -n 2 -ppn $ppn -l ./$example $backend $loop $coll` 2>&1 | tee ${test_log}
	else
		echo Output for run with CCL_LOG_LEVEL=2 has been redirected to log file ${test_log}
		eval `echo $ccl_extra_env mpiexec.hydra -n 2 -ppn $ppn -l ./$example $backend $loop $coll` > ${test_log} 2>&1
	fi
    check_test ${test_log} ${example}
}
run_example()
{
    echo "================ENVIRONMENT=================="
    local ccl_extra_env="$1"
    echo "ccl_extra_env: " "$ccl_extra_env"
    local dir_name=$2
    echo "dir_name: " $dir_name
    local transport=$3
    echo "transport: "$transport
    local example=$4
    echo "example: "$example
    local arg=$5
    echo "arg: "$arg
    echo "================ENVIRONMENT=================="
    test_log="$EXAMPLE_WORK_DIR/$dir_name/run_${dir_name}_${transport}_${example}_${arg}_output.log"
    eval `echo $ccl_extra_env mpiexec.hydra -n 2 -ppn $ppn -l ./$example $arg` 2>&1 | tee ${test_log}
    check_test ${test_log} ${example}
}

build()
{
    cd ${EXAMPLE_WORK_DIR}
    echo "Building"
    cmake .. -DCMAKE_DISABLE_SYCL=${DISABLE_SYCL} \
             -DCMAKE_C_COMPILER=${C_COMPILER} \
             -DCMAKE_CXX_COMPILER=${CXX_COMPILER}  2>&1 | tee ${EXAMPLE_WORK_DIR}/build_output.log
    make -j 2>&1 | tee -a ${EXAMPLE_WORK_DIR}/build_output.log
    error_count=`grep -E -c 'error:|Aborted|failed'  ${EXAMPLE_WORK_DIR}/build_output.log` > /dev/null 2>&1
    if [ "${error_count}" != "0" ]
    then
        echo "building ... NOK"
        echo "See logs ${EXAMPLE_WORK_DIR}/build_output.log"
        exit 1
    else
        echo "OK"
    fi
}
run()
{
    cd ${EXAMPLE_WORK_DIR}/
    pwd
    ppn=1
    n=2
    ccl_base_env="CCL_YIELD=sleep CCL_ATL_SHM=1"
    if [[ $is_sycl ==  1 ]];
    then
        dir_list="cpu sycl common"
        backend_list="cpu sycl"
    else
        dir_list="cpu common"
        backend_list="cpu"
    fi
    if ! [[ -n "${DASHBOARD_GPU_DEVICE_PRESENT}" ]]
    then
        echo "WARNING: DASHBOARD_GPU_DEVICE_PRESENT was not set"
        selectors_list="cpu host default"
    else
        selectors_list="cpu gpu host default"
    fi
    echo "DISABLE_SYCL =" $DISABLE_SYCL "; dir_list =" $dir_list "; selectors_list =" $selectors_list
    for dir_name in $dir_list
    do
        cd $dir_name
        pwd
        for transport in "ofi" "mpi";
        do
            ccl_transport_env="CCL_ATL_TRANSPORT=${transport} ${ccl_base_env}"

            if [ "$transport" == "mpi" ];
            then
                examples_to_run=`find . -type f -executable -printf '%P\n' | grep -v 'unordered_allreduce' | grep -v 'custom_allreduce' | grep -v 'datatype' | grep -v 'allreduce_rs'`
            else
                examples_to_run=`find . -type f -executable -printf '%P\n' | grep -v 'allreduce_rs'`
            fi
            
           
            coll_list="" # empty coll_list means default benchmarking collectives set
            for example in $examples_to_run
            do
                if [ "$dir_name" == "common" ];
                then
                    for backend in $backend_list
                    do                        
                        # MPI doesn't support sparse functionality, remove sparse_allreduce from coll_list
                        if [ "${transport}" == "mpi" ];
                        then
                            coll_list="allgatherv,allreduce,alltoall,alltoallv,bcast,reduce,allgatherv,allreduce,alltoall,alltoallv,bcast,reduce"
                        fi

                        ccl_extra_env="${ccl_transport_env}"

                        run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} regular ${coll_list}
                        
                        # run extended version of benchmark
                        if [[ "${example}" == *"benchmark"* ]]
                        then
                            for loop in "regular" "unordered"
                            do
                                if [ "$transport" == "mpi" ] && [ "$loop" == "unordered" ];
                                then
                                    continue
                                fi
                                ccl_extra_env="CCL_PRIORITY=lifo ${ccl_transport_env}"
                                run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${loop} ${coll_list}
                                ccl_extra_env="CCL_WORKER_OFFLOAD=0 ${ccl_transport_env}"
                                run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${loop} ${coll_list}
                            done
                            ccl_extra_env="CCL_FUSION=1 ${ccl_transport_env}"
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} regular allreduce
                            ccl_extra_env="CCL_LOG_LEVEL=2 ${ccl_transport_env}"
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} regular allreduce
                        fi
                    done
                elif [ "$dir_name" == "sycl" ];
                then
                    for selector in $selectors_list
                    do
                        if [ "$selector" == "gpu" ];
                        then
                            ccl_extra_env="SYCL_DEVICE_WHITE_LIST=\"\" SYCL_BE=PI_LEVEL0 ${ccl_transport_env}"
                            run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${selector}
                        fi
                        ccl_extra_env="SYCL_BE=PI_OPENCL ${ccl_transport_env}"
                        run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${selector}
                    done
                else
                    if [[ "${example}" == *"communicator"* ]]
                    then
                        n=8
                        ccl_extra_env="CCL_ALLREDUCE=recursive_doubling ${ccl_transport_env}"
                        run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example}                       
                    elif [[ "${example}" == *"sparse_allreduce"* ]]
                    then
                        for sparse_algo in "mask" "allgatherv";
                        do
                            ccl_extra_env="CCL_SPARSE_ALLREDUCE=$sparse_algo ${ccl_transport_env}"
                            run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example}
                        done
                    else
                        ccl_extra_env="${ccl_transport_env}"
                        run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example}
                    fi
                fi
            done
        done
        cd - > /dev/null 2>&1
    done
    if [ ${total_fails} != 0 ]
    then
        echo "There are ${total_fails} failed tests"
        sleep 20
        exit 1
    elif [ ${total_skipped} != 0 ]
    then
        echo "Tests passed, except ${total_skipped} skipped tests"
        sleep 20
        exit 0
    else
        echo "All tests passed!"
        sleep 20
        exit 0
    fi
}

print_help()
{
    echo_log "Usage:"
    echo_log "    ./${BASENAME}.sh <options>"
    echo_log "Example:"
    echo_log "    ./${BASENAME}.sh "
    echo_log "    ./${BASENAME}.sh cpu"
    echo_log "    ./${BASENAME}.sh gpu"
    echo_log ""
    echo_log "Available knobs:"
    echo_log "DISABLE_SYCL = 0|1, default is 0"
    echo_log "C_COMPILER = clang|icc|gcc|<full path to compiler>, default is clang"
    echo_log "CXX_COMPILER = clang++|icpc|g++|<full path to compiler>, default is clang++"
    echo_log ""
    echo_log "<options>:"
    echo_log "    -h|-H|-help|--help"
    echo_log "        Print help information"
    echo_log ""
}

case $1 in
"cpu" )
    if [ -z "${C_COMPILER}"]
    then
        C_COMPILER=gcc
    fi
    if [ -z "${CXX_COMPILER}"]
    then
        CXX_COMPILER=g++
    fi
    DISABLE_SYCL=1
    shift
    ;;
"--help|-help|-h" )
    print_help
    shift
    ;;
"gpu"|* )
    check_clang
    if [ -z "${C_COMPILER}"]
    then
        C_COMPILER=clang
    fi
    if [ -z "${CXX_COMPILER}"]
    then
        CXX_COMPILER=clang++
    fi
    DISABLE_SYCL=0
    shift
    ;;
esac

create_work_dir
build
run
