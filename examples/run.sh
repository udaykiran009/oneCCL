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

function CheckTest(){
    test_log=$1
    test_file=$2
    test_passed=`grep -c "PASSED" ${test_log}`
    if [ $test_file != "communicator.out" ]
    then
        test_failed=`grep -E -c -i 'error|Aborted|failed|^BAD$|KILLED|^fault$|cl::sycl::runtime_error|terminate' ${test_log}`
    else
        test_failed=`grep -E -c -i 'Aborted|failed|^BAD$|KILLED|^fault$|cl::sycl::runtime_error|terminate' ${test_log}`
    fi
    test_skipped=`grep -c "Accelerator is the first in device list, but unavailable" ${test_log}`
    if [ ${test_passed} -ne 1 ] || [ ${test_failed} -ne 0 ]
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

# export FI_PROVIDER=tcp - don't use OFI knob till fix of issue with OFI_GETINFO_HIDDEN on IMPI/libfabric level
export CCL_ATL_OFI_PROVIDER=tcp

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
    eval `echo $ccl_extra_env mpiexec.hydra -genv $EXTRA_ENV -n 2 -ppn $ppn -l ./$example $backend $loop $coll` 2>&1 | tee ${test_log}

    CheckTest ${test_log} ${example}
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
    echo "================ENVIRONMENT=================="
    test_log="$SCRIPT_DIR/$dir_name/run_${dir_name}_${transport}_${example}_output.log"
    eval `echo $ccl_extra_env mpiexec.hydra -genv $EXTRA_ENV -n 2 -ppn $ppn -l ./$example` 2>&1 | tee ${test_log}
    CheckTest ${test_log} ${example}
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
    echo "is_sycl =" $is_sycl "dir_list =" $dir_list
    for dir_name in $dir_list
    do
        cd $dir_name
        build $dir_name
        for transport in "ofi" "mpi";
        do
            if [ "$transport" == "mpi" ];
            then
                examples_to_run=`ls . | grep '.out' | grep -v '.log' | grep -v 'unordered_allreduce' | grep -v 'custom_allreduce'`
            else
                examples_to_run=`ls . | grep '.out' | grep -v '.log'`
            fi
            for example in $examples_to_run
            do
                if [ "$dir_name" == "common" ];
                then
                    for backend in $backend_list
                    do
                        ccl_extra_env="CCL_ATL_TRANSPORT=${transport}"
                        run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} 
                        # run extended version of benchmark
                        if [[ "${example}" == *"benchmark"* ]]
                        then
                            for loop in "regular" "unordered"
                            do
                                if [ "$transport" == "mpi" ] && [ "$loop" == "unordered" ];
                                then
                                    continue
                                fi
                                ccl_extra_env="CCL_PRIORITY=lifo CCL_ATL_TRANSPORT=${transport}"
                                run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${loop}
                                ccl_extra_env="CCL_WORKER_OFFLOAD=0 CCL_ATL_TRANSPORT=${transport}"
                                run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${loop}
                            done
                            ccl_extra_env="CCL_FUSION=1 CCL_ATL_TRANSPORT=${transport}"
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} regular allreduce
                            ccl_extra_env="CCL_LOG_LEVEL=2 CCL_ATL_TRANSPORT=${transport}"
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} regular allreduce
                        fi
                    done
                elif [ "$dir_name" == "sycl" ];
                then
                    for selector in "cpu" "gpu" "host" "default"
                    do
                        test_log="$SCRIPT_DIR/$dir_name/run_${dir_name}_${transport}_${selector}_${example}_output.log"
                        echo "run sycl examples with $transport transport and selector $selector (${example})" 2>&1 | tee ${test_log}
                        CCL_ATL_TRANSPORT=${transport} mpiexec.hydra -genv $EXTRA_ENV -n 2 -ppn $ppn -l ./$example $selector 2>&1 | tee ${test_log}
                        CheckTest ${test_log} ${example}
                    done
                else
                    if [[ "${example}" == *"bfp16"* ]]
                    then
                        ccl_extra_env="CCL_ATL_TRANSPORT=ofi"
                        run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example}
                    elif [[ "${example}" == *"communicator"* ]]
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
        exit 1
    elif [ ${total_skipped} != 0 ]
    then
        echo "Tests passed, except ${total_skipped} skipped tests"
        exit 0
    else
        echo "All tests passed!"
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
