#!/bin/bash

touch main_ouput.txt
exec | tee ./main_ouput.txt

BASENAME=`basename $0 .sh`

set_example_work_dir()
{
    SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`
    export EXAMPLE_WORK_DIR=${SCRIPT_DIR}/build
}

create_work_dir()
{
    if [[ -z "${EXAMPLE_WORK_DIR}" ]]
    then
        set_example_work_dir
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

check_test()
{
    local test_log=$1
    local test_file=$2
    test_passed=`grep -E -c -i 'PASSED' ${test_log}`
    if [[ "${test_file}" != *"communicator"* ]] && [[ "${test_file}" != *"datatype"* ]];
    then
        test_failed=`grep -E -c -i 'error|invalid|Aborted|fail|failed|^BAD$|KILLED|^fault$|cl::sycl::runtime_error|terminate' ${test_log}`
    else
        test_failed=`grep -E -c -i 'Aborted|invalid|fail|failed|^BAD$|KILLED|^fault$|cl::sycl::runtime_error|terminate' ${test_log}`
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

check_environment()
{
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
}

# global variable for several use places
# See: MLSL-675.
supported_kernels_colls="allgatherv,allreduce,alltoallv,bcast,reduce"
kernel_colls_with_dtypes="allreduce,reduce,allgatherv"
kernels_dtypes_list="int8,int32,float32"
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
    local runtime=$6
    echo "runtime: "$runtime
    local loop=$7
    echo "loop: "$loop
    local coll=$8
    echo "coll: " $coll
    local dtype=$9
    echo "dtype: " $dtype
    local reduction=${10}
    echo "reduction: " $reduction
    echo "================ENVIRONMENT=================="

    log_idx=${log_idx}+1
    base_test_log="$EXAMPLE_WORK_DIR/$dir_name/run"
    base_test_log="${base_test_log}_${transport}_${example}_b_${backend}_r_${runtime}_e_${loop}_l_${coll}_d_${dtype}_${log_idx}"

    options="--min_elem_count 1 --max_elem_count 32"
    usm_list="none"

    if [ "${backend}" != "" ];
    then
        options="${options} --backend ${backend}"

        if [ "${backend}" == "sycl" ];
        then
            buf_count=2
            if [ "${runtime}" == "level_zero" ];
            then
                # FIXME: enable back buf_count > 1 after USM fix in L0
                # https://jira.devtools.intel.com/browse/CMPLRLLVM-22660
                buf_count=1
            fi
            options="${options} --iters 8 --buf_count ${buf_count} --sycl_mem_type usm"
            usm_list="device shared"
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

    use_kernel=0
    # these conditions check for benchmark  with kernel
    # coll are all there except alltoall,reduce_scatter
    if [ "$example" == "benchmark" ] &&   \
       [ "$backend" == "sycl" ] &&        \
       [ "$transport" == "ofi" ] &&       \
       [ "$runtime" == "level_zero" ] &&  \
       [ "$loop" == "regular" ] &&        \
       [[ "$coll" == ${supported_kernels_colls} || "$coll" == ${kernel_colls_with_dtypes} ]] && \
       [[ "$dtype" == "float32" || "$dtype" == ${kernels_dtypes_list} ]] &&       \
       [ "$reduction" == "sum" ]
    then
        echo "set flag use_kernel"
        use_kernel=1
    fi

    for usm in $usm_list;
    do
        echo "usm: " $usm

        local test_log="${base_test_log}_${usm}.log"

        if [ "${backend}" == "sycl" ];
        then
            final_options="${options} --sycl_usm_type ${usm}"
        else
            final_options="${options}"
        fi

        if [ `echo $ccl_extra_env | grep -c CCL_LOG_LEVEL` -ne 1 ]
        then
            cmd=`echo $ccl_extra_env mpiexec.hydra -n 2 -ppn $ppn -l ./$example ${final_options}`
            echo "Running: $cmd"
            eval $cmd 2>&1 | tee ${test_log}
            check_test ${test_log} ${example}
        else
            log_2_test_log="${test_log}.2_ranks"
            echo "output for run with CCL_LOG_LEVEL=2 and 2 ranks has been redirected to log file ${log_2_test_log}"
            cmd=`echo $ccl_extra_env mpiexec.hydra -n 2 -ppn $ppn -l ./$example ${final_options}`
            echo "Running: $cmd"
            eval $cmd > ${log_2_test_log} 2>&1
            check_test ${log_2_test_log} ${example}

            if [ "${transport}" == "ofi" ];
            then
                log_2_test_log="${test_log}.1_rank"
                echo "output for run with CCL_LOG_LEVEL=2 and 1 rank has been redirected to log file ${log_2_test_log}"
                cmd=`echo $ccl_extra_env mpiexec.hydra -n 1 -ppn $ppn -l ./$example ${final_options}`
                echo "Running: $cmd"
                eval $cmd > ${log_2_test_log} 2>&1
                check_test ${log_2_test_log} ${example}
            fi
        fi
        if [ $use_kernel -eq 1 ]
        then
            echo "Launched benchmark with the kernels:"
            final_options="${options} -p 1 -k 4 -f 256 -t 512"
            cmd=`echo $ccl_extra_env CCL_COLL_KERNELS_PATH=../../../../src/kernels CCL_KVS_GET_TIMEOUT=10 ./$example ${final_options}`
            eval $cmd 2>&1 | tee ${test_log}
            check_test ${test_log} ${example}
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

    cmd=`echo $ccl_extra_env mpiexec.hydra -n 2 -ppn $ppn -l ./$example $arg`
    echo "Running $cmd"
    eval $cmd 2>&1 | tee ${test_log}
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
    dtype_list="int8,int32,float32"
    reduction_list="sum,max"
    ccl_base_env="FI_PROVIDER=tcp"

    if [[ ${MODE} = "cpu" ]]
    then
        dir_list="cpu common benchmark external_launcher"
        bench_backend_list="host"
        example_selector_list="cpu host default"
    else
        if [[ ${SCOPE} = "pr" ]]
        then
            dir_list="common benchmark"
            bench_backend_list="sycl"
            example_selector_list="host gpu"
        else
            dir_list="cpu common sycl benchmark"
            bench_backend_list="host sycl"
            example_selector_list="cpu host gpu default"
        fi
    fi

    echo "dir_list =" $dir_list "; example_selector_list =" $example_selector_list
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
                    grep -v 'communicator' |
                    grep -v 'sparse_allreduce' |
                    grep -v 'run_binary.sh' |
                    grep -v 'run.sh' |
                    grep -v 'external_launcher'`
            else
                examples_to_run=`find . -type f -executable -printf '%P\n' |
                    grep -v 'sparse_allreduce' |
                    grep -v 'run_binary.sh' |
                    grep -v 'run.sh'`
            fi

            for example in $examples_to_run
            do
                if [ "$dir_name" == "benchmark" ];
                then

                    coll_list="all"

                    for backend in $bench_backend_list
                    do
                        if [ "$backend" == "sycl" ];
                        then
                            runtime_list="opencl level_zero"
                        else
                            runtime_list="none"
                        fi

                        for runtime in $runtime_list
                        do
                            ccl_runtime_env="${ccl_transport_env}"

                            if [ "$runtime" == "opencl" ];
                            then
                                ccl_runtime_env="SYCL_BE=PI_OPENCL ${ccl_runtime_env}"
                            elif [ "$runtime" == "level_zero" ];
                            then
                                ccl_runtime_env="SYCL_BE=PI_LEVEL_ZERO ${ccl_runtime_env}"
                            fi

                            ccl_extra_env="${ccl_runtime_env}"
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${runtime} regular ${coll_list}

                            for loop in "regular" "unordered"
                            do
                                if [ "$transport" == "mpi" ] && [ "$loop" == "unordered" ];
                                then
                                    continue
                                fi
                                ccl_extra_env="CCL_PRIORITY=lifo ${ccl_runtime_env}"
                                run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${runtime} ${loop} ${coll_list}
                                ccl_extra_env="CCL_WORKER_OFFLOAD=0 ${ccl_runtime_env}"
                                run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${runtime} ${loop} ${coll_list}
                            done

                            ccl_extra_env="CCL_FUSION=1 ${ccl_runtime_env}"
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${runtime} regular allreduce

                            ccl_extra_env="CCL_LOG_LEVEL=2 ${ccl_runtime_env}"
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${runtime} regular ${coll_list}
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${runtime} regular allreduce ${dtype_list} ${reduction_list}
                            ccl_extra_env="${ccl_runtime_env}"
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${runtime} regular ${supported_kernels_colls} float32 sum
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${runtime} regular ${kernel_colls_with_dtypes} ${kernels_dtypes_list} sum
                        done
                    done
                elif [ "$dir_name" == "sycl" ];
                then
                    for selector in $example_selector_list
                    do
                        if [[ "${example}" == *"_usm_"* ]]
                        then
                            usm_list="host device shared"
                        else
                            usm_list="default"
                        fi

                        echo "usm_list: $usm_list"

                        for usm in $usm_list
                        do
                            if [ "$usm" == "host" ] && [ "$selector" == "gpu" ];
                            then
                                continue
                            fi

                            if [ "$usm" == "device" ] && [ "$selector" != "gpu" ];
                            then
                                continue
                            fi

                            echo "selector $selector, usm $usm"

                            if [ "$selector" == "gpu" ];
                            then
                                ccl_extra_env="SYCL_BE=PI_LEVEL_ZERO ${ccl_transport_env}"
                                run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example} "${selector} ${usm}"
                            fi
                            ccl_extra_env="SYCL_BE=PI_OPENCL ${ccl_transport_env}"
                            run_example "${ccl_extra_env}" ${dir_name} ${transport} ${example} "${selector} ${usm}"
                        done
                    done
                elif [ "$dir_name" == "external_launcher" ];
                then
                    if [ "$transport" != "ofi" ];
                    then
                        continue
                    fi

                    if [ -z "${I_MPI_HYDRA_HOST_FILE}" ];
                    then
                        echo "Error: I_MPI_HYDRA_HOST_FILE was not set."
                        exit 1
                    else
                        ccl_hosts=`echo $I_MPI_HYDRA_HOST_FILE`
                        ccl_world_size=4
                        ccl_vars="${CCL_ROOT}/env/setvars.sh"
                        if [ ! -f "$ccl_vars" ];
                        then
                            # not a standalone version
                            ccl_vars="${CCL_ROOT}/env/vars.sh"
                        fi

                        test_log="$EXAMPLE_WORK_DIR/$dir_name/log.txt"
                        ${EXAMPLE_WORK_DIR}/${dir_name}/run.sh -v $ccl_vars -h $ccl_hosts -s $ccl_world_size 2>&1 | tee ${test_log}
                        check_test ${test_log} "external_launcher"
                    fi
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
    echo_log "Usage: ${BASENAME}.sh <options>"
    echo_log "  --mode mode             cpu|gpu mode"
    echo_log "  --scope scope           pr reduces scope till minimal pre-merge scope"
    echo_log "  --build-only            build only"
    echo_log "  --help                  print help information"
    echo_log
    echo_log "Usage examples:"
    echo_log "  ${BASENAME}.sh "
    echo_log "  ${BASENAME}.sh --mode cpu"
    echo_log "  ${BASENAME}.sh --mode gpu"
    echo_log "  ${BASENAME}.sh --mode gpu --scope pr"
    echo_log ""
    echo_log "Available knobs:"
    echo_log "  C_COMPILER = clang|icc|gcc|<full path to compiler>, default is clang"
    echo_log "  CXX_COMPILER = clang++|icpc|g++|<full path to compiler>, default is clang++"
    echo_log ""
}

parse_arguments()
{
    while [ $# -ne 0 ]
    do
        case $1 in
            "-h" | "--help" | "-help")                  print_help ; exit 0 ;;
            "--mode" | "-mode")                         MODE=$2 ; shift ;;
            "gpu")                                      MODE="gpu" ;;
            "cpu")                                      MODE="cpu" ;;
            "pr")                                       SCOPE="pr" ;;
            "--mode" | "-mode")                         MODE=$2 ; shift ;;
            "--scope" | "-scope")                       SCOPE=$2 ; shift ;;
            "--build-only" | "-build-only")             BUILD_ONLY="yes" ;;
            "--run-only" | "-run-only")                 RUN_ONLY="yes" ;;
            *)
            echo "$(basename $0): ERROR: unknown option ($1)"
            print_help
            exit 1
            ;;
        esac
        shift
    done

    echo "Parameters:"
    [[ ! -z "$MODE" ]] && echo "MODE: $MODE"
    [[ ! -z "$SCOPE" ]] && echo "SCOPE: $SCOPE"
    [[ ! -z "$BUILD_ONLY" ]] && echo "BUILD_ONLY: $BUILD_ONLY"
    [[ ! -z "$RUN_ONLY" ]] && echo "RUN_ONLY: $RUN_ONLY"
    echo
}

check_mode()
{
    if [[ -z ${MODE} ]]
    then
        if [[ ! ",${DASHBOARD_INSTALL_TOOLS_INSTALLED}," == *",dpcpp,"* ]] || [[ ! -n ${DASHBOARD_GPU_DEVICE_PRESENT} ]]
        then
            echo "WARNING: DASHBOARD_INSTALL_TOOLS_INSTALLED variable doesn't contain 'dpcpp' or the DASHBOARD_GPU_DEVICE_PRESENT variable is missing"
            echo "WARNING: Using cpu_icc configuration of the library"
            source ${CCL_ROOT}/env/vars.sh --ccl-configuration=cpu_icc
            MODE="cpu"
        else
            MODE="gpu"
        fi
    fi

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
        ;;
    "gpu"|* )
        check_clang
        if [ -z "${C_COMPILER}" ]
        then
            C_COMPILER=clang
        fi
        if [ -z "${CXX_COMPILER}" ]
        then
            CXX_COMPILER=dpcpp
            COMPUTE_RUNTIME="dpcpp"
        fi
        ;;
    esac
}

check_scope()
{
    if [[ ! -z ${SCOPE} ]] && [[ ${SCOPE} != "pr" ]]
    then
        echo "Unsupported scope: $SCOPE"
        print_help
        exit 1
    fi
}

parse_arguments $@
check_environment
check_mode

if [[ ${RUN_ONLY} != "yes" ]]
then
    check_scope
    create_work_dir
    if [[ -z ${BUILD_ONLY} ]] || [[ ${BUILD_ONLY} == "no" ]]
    then
        build
    else
        build
        exit 0
    fi
fi

if [[ -z ${BUILD_ONLY} ]] || [[ ${BUILD_ONLY} == "no" ]]
then
    if [[ -z ${RUN_ONLY} ]] || [[ ${RUN_ONLY} == "no" ]]
    then
        run
    else
        set_example_work_dir
        run
        exit 0
    fi
fi
