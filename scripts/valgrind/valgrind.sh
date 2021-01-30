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
    # test_passed=`grep -E -c -i 'PASSED' ${test_log}`
    test_failed=`grep -E -c -i 'invalid|Aborted|fail|failed|^BAD$|KILLED|^fault$|cl::sycl::runtime_error|terminate' ${test_log}`
    # test_skipped=`grep -E -c -i 'unavailable|skipped|skip' ${test_log}`
    if [ ${test_failed} -ne 0 ]
    then
        echo "Error: example $test_file testing failed"
        echo "See log ${test_log} for details"
        total_fails=${total_fails}+1
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

set_debug_impi_env()
{
    source $I_MPI_ROOT/env/vars.sh --i_mpi_library_kind=debug_mt
    IMPI_DEBUG_CHECK=$?
    if [ "$IMPI_DEBUG_CHECK" != "0" ]
    then
        echo "Warning: $I_MPI_ROOT/env/vars.sh --i_mpi_library_kind=debug_mt wasn't sourced"
        exit 1
    fi
    echo "LD_LIBRARY_PATH = $LD_LIBRARY_PATH"
}

# global variable for several use places
# See: MLSL-675.
supported_kernel_colls="allgatherv,allreduce,alltoallv,bcast,reduce"
supported_kernel_colls_with_dtypes="allreduce,reduce,allgatherv"
supported_kernel_dtypes="int8,int32,float32"

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

    if [[ ${VALGRIND_SCOPE} = "regular" ]]
    then
        options=" -w 5 -i 100 -y 256,2048,1048576,2097152 -d all -r all"
    else
        options=" -w 0 -i 2 -n 2 -y 2048,1048576"
    fi

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
            options="${options} --sycl_mem_type usm"
            usm_list="device shared"
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

    use_kernels=0
    # these conditions check for benchmark with kernels
    # coll are all there except alltoall,reduce_scatter

    # all colls, reduce iters till 2-3, gpu and host selector, mpi and ofi, level_zero
    # nightly (more messages, mpi and ofi, opencl and l0) + CI scope (mpi, l0,)
    # debug mpi/ofi/ccl
    # test:vg
    # test:example (test:pr, test:all) + test:vg
    
    if [ "$example" == "benchmark" ] &&   \
       [ "$backend" == "sycl" ] &&        \
       [ "$transport" == "ofi" ] &&       \
       [ "$runtime" == "level_zero" ] &&  \
       [ "$loop" == "regular" ] &&        \
       [[ "$coll" == ${supported_kernel_colls} || "$coll" == ${supported_kernel_colls_with_dtypes} ]] && \
       [[ "$dtype" == "float32" || "$dtype" == ${supported_kernel_dtypes} ]] &&       \
       [ "$reduction" == "sum" ]
    then
        echo "set flag use_kernels"
        use_kernels=1
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
        cmd=`echo $ccl_extra_env mpiexec -hosts localhost -n 2 valgrind --show-leak-kinds=all ./$example ${final_options}`
        echo "Running: $cmd"
        eval $cmd 2>&1 | tee ${test_log}
        check_test ${test_log} ${example}
    done
}

build()
{
    cd ${EXAMPLE_WORK_DIR}
    # please use line below for manual testing by run.sh
    # cp ${EXAMPLE_WORK_DIR}/../../ccl_oneapi/CMakeLists.txt ${EXAMPLE_WORK_DIR}/../
    echo "Building"
    if [ -z "${COMPUTE_BACKEND}" ]
    then
        cmake .. -DCMAKE_C_COMPILER=${C_COMPILER} \
                 -DCMAKE_CXX_COMPILER=${CXX_COMPILER} 2>&1 | tee ${EXAMPLE_WORK_DIR}/build_output.log
    else
        cmake .. -DCMAKE_C_COMPILER=${C_COMPILER} \
                 -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
                 -DCOMPUTE_BACKEND=${COMPUTE_BACKEND} 2>&1 | tee ${EXAMPLE_WORK_DIR}/build_output.log
    fi
    make -j  benchmark 2>&1 | tee -a ${EXAMPLE_WORK_DIR}/build_output.log
    error_count=`grep -E -i -c 'invalid|abort|fail'  ${EXAMPLE_WORK_DIR}/build_output.log` > /dev/null 2>&1
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
            dir_list="benchmark"
            bench_backend_list="host"
    else
            dir_list="benchmark"
            bench_backend_list="sycl"
    fi
    if [[ ${VALGRIND_SCOPE} = "regular" ]]
    then
        runtime_list="level_zero opencl"
        transport_list="ofi mpi"
    else
        runtime_list="level_zero"
        transport_list="mpi"
    fi
    for dir_name in $dir_list
    do
        if [ ! -d "$(pwd)/${dir_name}" ]
        then
            echo "Directory $(pwd)/${dir_name} not found"
            continue
        fi

        cd $dir_name
        pwd
        for transport in ${transport_list};
        do
            ccl_transport_env="CCL_ATL_TRANSPORT=${transport} ${ccl_base_env}"
            coll_list="all"
            example="benchmark"

            for backend in $bench_backend_list
            do
                if [ "$backend" == "sycl" ];
                then
                    runtime_list=$runtime_list
                else
                    runtime_list="none"
                fi

                for runtime in $runtime_list
                do
                    ccl_runtime_env="${ccl_transport_env}"

                    if [ "$runtime" == "opencl" ];
                    then
                        ccl_runtime_env="SYCL_DEVICE_FILTER=opencl:*:0 ${ccl_runtime_env}"
                    elif [ "$runtime" == "level_zero" ];
                    then
                        ccl_runtime_env="SYCL_DEVICE_FILTER=level_zero:*:0 ${ccl_runtime_env}"
                    fi

                    ccl_extra_env="${ccl_runtime_env}"
                    run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${runtime} regular ${coll_list}
                done
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
    echo_log "  --mode mode               cpu|gpu mode"
    echo_log "  --scope scope             short|regular scope"
    echo_log "  --help                    print help information"
    echo_log
    echo_log "Usage examples:"
    echo_log "  ${BASENAME}.sh "
    echo_log "  ${BASENAME}.sh --mode cpu --scope short"
    echo_log "  ${BASENAME}.sh --mode gpu"
    echo_log ""
    echo_log "Available knobs:"
    echo_log "  C_COMPILER = clang|icc|gcc|<full path to compiler>, default is clang"
    echo_log "  CXX_COMPILER = clang++|icpc|g++|<full path to compiler>, default is clang++"
    echo_log ""
}

check_scope()
{
    if [[ ! -z ${VALGRIND_SCOPE} ]]
    then
        case $VALGRIND_SCOPE in
            "short" | "regular") ;;
            *)
            echo "Unsupported scope: $VALGRIND_SCOPE"
            print_help
            exit 1
            ;;
        esac
    fi
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
            "--mode" | "-mode")                         MODE=$2 ; shift ;;
            "--scope" | "-scope")                       VALGRIND_SCOPE=$2 ; shift ;;
            "short"             )                       VALGRIND_SCOPE="short" ;;
            "regular"           )                       VALGRIND_SCOPE="regular" ;;
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
            COMPUTE_BACKEND="dpcpp"
        fi
        ;;
    esac
}

parse_arguments $@
check_environment
set_debug_impi_env
check_mode
check_scope
create_work_dir
build
run
