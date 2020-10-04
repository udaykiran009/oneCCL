#!/bin/bash

touch main_ouput.txt
exec | tee ./main_ouput.txt

BASENAME=`basename $0 .sh`

function create_work_dir()
{
    if [ -z ${EXAMPLE_WORK_DIR} ]
    then
        SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
        export EXAMPLE_WORK_DIR=${SCRIPT_DIR}/build
        rm -r ${EXAMPLE_WORK_DIR}
        mkdir -p ${EXAMPLE_WORK_DIR}
    fi
    echo "EXAMPLE_WORK_DIR =" ${EXAMPLE_WORK_DIR}
}

CheckCommandExitCode()
{
    if [ $1 -ne 0 ]; then
        echo "ERROR: ${2}" 1>&2
        exit $1
    fi
}

declare -i total_fails=0
declare -i total_skipped=0
declare -i log_idx=0

function check_test()
{
    local test_log=$1
    local test_file=$2
    test_passed=`grep -E -c -i 'PASSED' ${test_log}`
    if [[ "${test_file}" != *"communicator"* ]] && [[ "${test_file}" != *"datatype"* ]];
    then
        test_failed=`grep -E -c -i 'error|Aborted|failed|^BAD$|KILLED|^fault$|cl::sycl::runtime_error|terminate' ${test_log}`
    else
        test_failed=`grep -E -c -i 'Aborted|failed|^BAD$|KILLED|^fault$|cl::sycl::runtime_error|terminate' ${test_log}`
    fi
    test_skipped=`grep -E -c -i 'unavailable|skipped|skip' ${test_log}`
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

check_clang()
{
    which clang++
    COMPILER_INSTALL_CHECK=$?
    if [ "$COMPILER_INSTALL_CHECK" != "0" ]
    then
        echo "Warning: clang++ compiler wasn't found, ccl-configuration=cpu_icc will be sourced"
        source ${CCL_ROOT}/env/vars.sh --ccl-configuration=cpu_icc
        MODE="cpu"
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
    local dtype=$8
    echo "dtype: " $dtype
    local reduction=$9
    echo "reduction: " $reduction
    echo "================ENVIRONMENT=================="

    log_idx=${log_idx}+1
    base_test_log="$EXAMPLE_WORK_DIR/$dir_name/run"
    base_test_log="${base_test_log}_${transport}_${example}_b_${backend}_e_${loop}_l_${coll}_d_${dtype}_${log_idx}"

    options="--min_elem_count 1 --max_elem_count 32"

    if [ "${backend}" != "" ];
    then
        options="${options} --backend ${backend}"

        if [ "${backend}" == "sycl" ];
        then
            options="${options} --iters 4 --buf_count 2"
        else
            options="${options} --iters 16 --buf_count 8"
        fi
    fi
    if [ "${loop}" != "" ];
    then
        options="${options} --loop ${loop}"
    fi
    if [ "${coll}" != "" ];
    then
        options="${options} --coll ${coll}"
    fi
    if [ "${dtype}" != "" ];
    then
        options="${options} --dtype ${dtype}"
    fi
    if [ "${reduction}" != "" ];
    then
        options="${options} --reduction ${reduction}"
    fi

    for buf_type in "single" "multi"
    do
        local test_log="${base_test_log}_${buf_type}.log"
        final_option="${options} --buf_type ${buf_type}"
        if [ `echo $ccl_extra_env | grep -c CCL_LOG_LEVEL` -ne 1 ]
        then
            eval `echo $ccl_extra_env mpiexec.hydra -n 2 -ppn $ppn -l ./$example ${final_option}` 2>&1 | tee ${test_log}
            check_test ${test_log} ${example}
        else
            log_2_test_log="${test_log}.2_ranks"
            echo "output for run with CCL_LOG_LEVEL=2 and 2 ranks has been redirected to log file ${log_2_test_log}"
            eval `echo $ccl_extra_env mpiexec.hydra -n 2 -ppn $ppn -l ./$example ${final_option}` > ${log_2_test_log} 2>&1
            check_test ${log_2_test_log} ${example}

            if [ "${transport}" == "ofi" ];
            then
                log_2_test_log="${test_log}.1_rank"
                echo "output for run with CCL_LOG_LEVEL=2 and 1 rank has been redirected to log file ${log_2_test_log}"
                eval `echo $ccl_extra_env mpiexec.hydra -n 1 -ppn $ppn -l ./$example ${final_option}` > ${log_2_test_log} 2>&1
                check_test ${log_2_test_log} ${example}
            fi
        fi
    done
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

    log_idx=${log_idx}+1
    local test_log="$EXAMPLE_WORK_DIR/$dir_name/run_${dir_name}_${transport}_${example}_${log_idx}.log"

    eval `echo $ccl_extra_env mpiexec.hydra -n 2 -ppn $ppn -l ./$example $arg` 2>&1 | tee ${test_log}
    check_test ${test_log} ${example}
}

build()
{
    cd ${EXAMPLE_WORK_DIR}
    # please use line below for manual testing by run.sh
    # cp ${EXAMPLE_WORK_DIR}/../../ccl_oneapi/CMakeLists.txt ${EXAMPLE_WORK_DIR}/../
    echo "Building"
    if [ -z "${COMPUTE_RUNTIME}" ]
    then
        cmake .. -DCMAKE_C_COMPILER=${C_COMPILER} \
                 -DCMAKE_CXX_COMPILER=${CXX_COMPILER} 2>&1 | tee ${EXAMPLE_WORK_DIR}/build_output.log
    else
        cmake .. -DCMAKE_C_COMPILER=${C_COMPILER} \
                 -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
                 -DCOMPUTE_RUNTIME=${COMPUTE_RUNTIME}  2>&1 | tee ${EXAMPLE_WORK_DIR}/build_output.log
    fi
    make -j 2>&1 | tee -a ${EXAMPLE_WORK_DIR}/build_output.log
    error_count=`grep -E -i -c 'error|abort|fail'  ${EXAMPLE_WORK_DIR}/build_output.log` > /dev/null 2>&1
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
    dtype_list="char,int,float"
    reduction_list="sum,max"
    ccl_base_env="FI_PROVIDER=tcp CCL_YIELD=sleep"

    if [[ ${MODE} = "cpu" ]]
    then
        dir_list="cpu common benchmark"
        backend_list="host"
        selectors_list="cpu host default"
    else
        dir_list="cpu common sycl benchmark"
        backend_list="host sycl"
        selectors_list="cpu host gpu default"
    fi

    echo "dir_list =" $dir_list "; selectors_list =" $selectors_list
    for dir_name in $dir_list
    do
        cd $dir_name
        pwd
        for transport in "ofi" "mpi";
        do
            ccl_transport_env="CCL_ATL_TRANSPORT=${transport} ${ccl_base_env}"

            if [ "$transport" == "mpi" ];
            then
                examples_to_run=`find . -type f -executable -printf '%P\n' |
                    grep -v 'unordered_allreduce' |
                    grep -v 'custom_allreduce' |
                    grep -v 'datatype' |
                    grep -v 'allreduce_rs' |
                    grep -v 'communicator' |
                    grep -v 'sparse_allreduce'`
            else
                examples_to_run=`find . -type f -executable -printf '%P\n' |
                    grep -v -e '\(^allreduce_rs$\|^platform_info$\)' |
                    grep -v 'sparse_allreduce'`
            fi

            coll_list="" # empty coll_list means default benchmarking collectives set
            for example in $examples_to_run
            do
                if [ "$dir_name" == "benchmark" ];
                then
                    for backend in $backend_list
                    do
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
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} regular

                            ccl_extra_env="${ccl_transport_env}"
                            # run a benchmark with the specific datatypes and reductions
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} regular allreduce ${dtype_list} ${reduction_list}
                        fi
                    done
                elif [ "$dir_name" == "sycl" ];
                then
                    for selector in $selectors_list
                    do
                        if [ "$selector" == "gpu" ];
                        then
                            ccl_extra_env="SYCL_DEVICE_ALLOWLIST=\"\" SYCL_BE=PI_LEVEL0 ${ccl_transport_env}"
                            run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${selector}
                        fi
                        ccl_extra_env="SYCL_BE=PI_OPENCL ${ccl_transport_env}"
                        run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${selector}
                    done
                else
                    if [[ "${example}" == *"communicator"* ]]
                    then
                        n=8
                        ccl_extra_env="${ccl_transport_env}"
                        run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example}
                    elif [[ "${example}" == *"sparse_allreduce"* ]]
                    then
                        if [ "$transport" == "mpi" ];
                        then
                            sparse_algo_set="ring"
                            sparse_coalesce_set="regular"
                            sparse_callback_set="completion"
                        else
                            sparse_algo_set="ring mask allgatherv"
                            sparse_coalesce_set="regular disable keep_precision"
                            sparse_callback_set="completion alloc"
                        fi
                        for algo in ${sparse_algo_set};
                        do
                            ccl_extra_env="CCL_SPARSE_ALLREDUCE=${algo} ${ccl_transport_env}"
                            for coalesce in ${sparse_coalesce_set};
                            do
                                for callback in ${sparse_callback_set};
                                do
                                    if [ "$callback" == "alloc" ] && [ "$algo" != "allgatherv" ];
                                    then
                                        # TODO: implement alloc_fn for ring and mask
                                        continue
                                    fi
                                    sparse_options="-coalesce ${coalesce} -callback ${callback}"
                                    run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example} "${sparse_options}"
                                done
                            done
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

echo_log()
{
    echo -e "$*"
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
    echo_log "C_COMPILER = clang|icc|gcc|<full path to compiler>, default is clang"
    echo_log "CXX_COMPILER = clang++|icpc|g++|<full path to compiler>, default is clang++"
    echo_log ""
    echo_log "<options>:"
    echo_log "    -h Print help information"
    echo_log ""
}

if [[ ! -z ${1} ]]
then
    MODE=${1}
elif [[ ! ",${DASHBOARD_INSTALL_TOOLS_INSTALLED}," == *",dpcpp,"* ]] || [[ ! -n ${DASHBOARD_GPU_DEVICE_PRESENT} ]]
then
    echo "WARNING: DASHBOARD_INSTALL_TOOLS_INSTALLED variable doesn't contain 'dpcpp' or the DASHBOARD_GPU_DEVICE_PRESENT variable is missing"
    echo "WARNING: Using cpu_icc configuration of the library"
    source ${CCL_ROOT}/env/vars.sh --ccl-configuration=cpu_icc
    MODE="cpu"
else
    MODE="gpu"
fi
echo "MODE: $MODE "

case $MODE in
"cpu" )
    if [[ -z "${C_COMPILER}" ]] && [[ ",${DASHBOARD_INSTALL_TOOLS_INSTALLED}," == *",icc,"* ]]
    then
        C_COMPILER=icc
    else
        C_COMPILER=gcc
    fi
    if [[ -z "${CXX_COMPILER}" ]] && [[ ",${DASHBOARD_INSTALL_TOOLS_INSTALLED}," == *",icc,"* ]]
    then
        CXX_COMPILER=icpc
    else
        CXX_COMPILER=g++
    fi
    shift
    ;;
"-h" )
    print_help
    exit 0
    ;;
"gpu"|* )
    check_clang
    if [ -z "${C_COMPILER}"]
    then
        C_COMPILER=clang
    fi
    if [ -z "${CXX_COMPILER}"]
    then
        CXX_COMPILER=dpcpp
        COMPUTE_RUNTIME="dpcpp"
    fi
    shift
    ;;
esac

create_work_dir
build
run
