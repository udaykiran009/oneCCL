#!/bin/bash

touch main_ouput.txt
exec | tee ./main_ouput.txt
SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`

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
    test_passed=`grep -E -c -i 'PASSED|skipped' ${test_log}`
    if [ $test_file != "communicator.out" ]
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
        echo "GPU test for example  $test_file has been skipped, default selector used instead."
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
    test_log="$SCRIPT_DIR/$dir_name/run_${transport}_${example}_${backend}_${loop}_output.log"
	if [ `echo $ccl_extra_env | grep -c CCL_LOG_LEVEL` -ne 1 ]
	then
		eval `echo $ccl_extra_env mpiexec.hydra -genv $EXTRA_ENV -n 2 -ppn $ppn -l ./$example $backend $loop $coll` 2>&1 | tee ${test_log}
	else
		echo Output for run with CCL_LOG_LEVEL=2 has been redirected to log file ${test_log}
		eval `echo $ccl_extra_env mpiexec.hydra -genv $EXTRA_ENV -n 2 -ppn $ppn -l ./$example $backend $loop $coll` > ${test_log} 2>&1
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
    test_log="$SCRIPT_DIR/$dir_name/run_${dir_name}_${transport}_${example}_${arg}_output.log"
    eval `echo $ccl_extra_env mpiexec.hydra -genv $EXTRA_ENV -n 2 -ppn $ppn -l ./$example $arg` 2>&1 | tee ${test_log}
    check_test ${test_log} ${example}
}

build()
{
    local dir_name="$1"
    echo "make clean"
    make clean -f ./../Makefile
    echo "Building"
    ENABLE_SYCL=$is_sycl make all -f ./../Makefile  2>&1 | tee $SCRIPT_DIR/$dir_name/build_${dir_name}_output.log
    error_count=`grep -E -c 'error:|Aborted|failed'  $SCRIPT_DIR/$dir_name/build_${dir_name}_output.log` > /dev/null 2>&1
    if [ "${error_count}" != "0" ]
    then
        echo "building ... NOK"
        echo "See logs $SCRIPT_DIR/$dir_name/build_${dir_name}_output.log"
        exit 1
    else
        echo "OK"
    fi
}
run()
{
    ppn=1
    n=2
    EXTRA_ENV="CCL_YIELD=sleep"
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
    echo "is_sycl =" $is_sycl "; dir_list =" $dir_list "; selectors_list =" $selectors_list
    for dir_name in $dir_list
    do
        cd $dir_name
        build $dir_name
        for transport in "ofi" "mpi";
        do
            if [ "$transport" == "mpi" ];
            then
                examples_to_run=`find . -type f -executable -printf '%P\n' | grep -v 'unordered_allreduce' | grep -v 'custom_allreduce' | grep -v 'allreduce_rs'`
            else
                examples_to_run=`find . -type f -executable -printf '%P\n' | grep -v 'allreduce_rs'`
            fi
            
           
            colls_list=""        # use empty coll list means default benchmarking collectives set
            offload="1"          #default value of CCL_WORKER_OFFLOAD
            for example in $examples_to_run
            do
                if [ "$dir_name" == "common" ];
                then
                    for backend in $backend_list
                    do
                        ccl_extra_env="CCL_ATL_TRANSPORT=${transport}"
                        base_benchmark_extra_env="${ccl_extra_env}"
                        
                        # MPI provide a limited sparse functionality:
                        # Limitations: CCL_WORKER_OFFLOAD=0 and no sparse duplicates in coll_list
                        if [ "${transport}" == "mpi" ];
                        then
                            # override environment values due to limitations
                            offload="0"
                            base_benchmark_extra_env="CCL_WORKER_OFFLOAD=${offload} ${base_benchmark_extra_env}"
                            colls_list="allgatherv,allreduce,alltoall,alltoallv,bcast,reduce,sparse_bfp16_allreduce,allgatherv,allreduce,alltoall,alltoallv,bcast,reduce,sparse_bfp16_allreduce"
                        fi
                        run_benchmark "${base_benchmark_extra_env}" ${dir_name} ${transport} ${example} ${backend} "regular" ${colls_list}
                        
                        # run extended version of benchmark
                        if [[ "${example}" == *"benchmark"* ]]
                        then
                            for loop in "regular" "unordered"
                            do
                                if [ "$transport" == "mpi" ] && [ "$loop" == "unordered" ];
                                then
                                    continue
                                fi
                                ccl_extra_env="CCL_WORKER_OFFLOAD=${offload} CCL_PRIORITY=lifo CCL_ATL_TRANSPORT=${transport}"
                                run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${loop} ${colls_list}
                                ccl_extra_env="CCL_WORKER_OFFLOAD=0 CCL_ATL_TRANSPORT=${transport}"
                                run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${loop} ${colls_list}
                            done
                            ccl_extra_env="CCL_WORKER_OFFLOAD=${offload} CCL_FUSION=1 CCL_ATL_TRANSPORT=${transport}"
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} regular allreduce
                            ccl_extra_env="CCL_WORKER_OFFLOAD=${offload} CCL_LOG_LEVEL=2 CCL_ATL_TRANSPORT=${transport}"
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} regular allreduce
                        fi
                    done
                elif [ "$dir_name" == "sycl" ];
                then
                    for selector in $selectors_list
                    do
                        if [ "$selector" == "gpu" ];
                        then
                            ccl_extra_env="SYCL_DEVICE_WHITE_LIST=\"\" SYCL_BE=PI_OTHER CCL_ATL_TRANSPORT=${transport}"
                            run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${selector}
                        fi
                        ccl_extra_env="SYCL_BE=OpenCL CCL_ATL_TRANSPORT=${transport}"
                        run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${selector}
                    done
                else
                    if [[ "${example}" == *"communicator"* ]]
                    then
                        n=8
                        ccl_extra_env="CCL_ALLREDUCE=recursive_doubling CCL_ATL_TRANSPORT=${transport}"
                        run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example}                       
                    elif [[ "${example}" == *"sparse_allreduce"* ]]
                    then
                        for sparse_algo in "mask" "allgatherv";
                        do
                            ccl_extra_env="CCL_SPARSE_ALLREDUCE=$sparse_algo CCL_ATL_TRANSPORT=${transport}"
                            run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example}
                        done
                    else
                        ccl_extra_env="CCL_ATL_TRANSPORT=${transport}"
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

case $1 in
"gpu" )
    check_clang
    is_sycl=1
    shift
    ;;
"cpu" )
    is_sycl=0
    shift
    ;;
* )
    check_clang
    is_sycl=1
    echo "WARNING: example testing will be running with DPC++"
    shift
    ;;
esac

run
