#!/bin/bash

touch main_ouput.txt
exec | tee ./main_ouput.txt

BASENAME=`basename $0 .sh`

if [[ ${node_label} == "ccl_test_cpu" && ${site} == "an" ]]
then
    VALGRIND_DIR="/home2/sys_ctlab/prog/valgrind/last"
else
    VALGRIND_DIR="/p/pdsd/opt/tools/valgrind/last"
fi
SCRIPT_DIR=$(cd $(dirname "$BASH_SOURCE") && pwd -P)
EXAMPLE_WORK_DIR=${SCRIPT_DIR}/../../examples/build

source ${SCRIPT_DIR}/../utils/common.sh

create_work_dir()
{
    if [[ -z "${EXAMPLE_WORK_DIR}" ]]
    then
        rm -r ${EXAMPLE_WORK_DIR}
        mkdir -p ${EXAMPLE_WORK_DIR}
    fi
    echo "EXAMPLE_WORK_DIR =" ${EXAMPLE_WORK_DIR}
}

declare -i total_fails=0
declare -i total_skipped=0
declare -i log_idx=0

check_test()
{
    local test_log=$1
    local test_file=$2
    # test_passed=`grep -E -c -i 'PASSED' ${test_log}`
    test_failed=`grep -E -c -i 'illegal|mismatched|invalid|Aborted|fail|failed|^BAD$|KILLED|^fault$|sycl::runtime_error|terminate' ${test_log}`
    # test_skipped=`grep -E -c -i 'unavailable|skipped|skip' ${test_log}`
    if [ ${test_failed} -ne 0 ]
    then
        echo "Error: example $test_file testing failed"
        echo "See log ${test_log} for details"
        total_fails=${total_fails}+1
    fi
}

check_dpcpp()
{
    which dpcpp
    COMPILER_INSTALL_CHECK=$?
    if [ "$COMPILER_INSTALL_CHECK" != "0" ]
    then
        set_dpcpp_compiler
    fi
}

check_and_set_impi_path()
{
    if [ -z "${I_MPI_ROOT}" ]
    then
        echo "$(hostname)" > "${SCRIPT_DIR}/hostfile"
        export I_MPI_HYDRA_HOST_FILE="${SCRIPT_DIR}/hostfile"
        set_impi_environment
    fi
}

check_environment()
{
    check_and_set_impi_path
    if [[ -z ${CCL_ROOT} ]]
    then
        if [[ ${node_label} == "ccl_test_cpu" && ${site} == "an" ]]; then
            CCL_ROOT=$(find ${JFCST_DEV_WORKSPACE_DIR}/${BUILDER_NAME}/${MLSL_BUILD_ID} -name "l_ccl_${build_type}*" -type d)
        else
            CCL_ROOT=$(find ${ARTEFACT_DIR} -name "l_ccl_${build_type}*" -type d)
        fi
        echo "CCL_ROOT='${CCL_ROOT}'"; export CCL_ROOT
    fi

    export PATH="${VALGRIND_DIR}/bin:${PATH}"
    export VALGRIND_LIB="${VALGRIND_DIR}/libexec/valgrind/"
    echo "VALGRIND_LIB='${VALGRIND_LIB}'"
    which valgrind
    valgrind --version
    vg_major_version=`valgrind --version | awk -F"[-.]" '{print $2}'`
    vg_minor_version=`valgrind --version | awk -F"[-.]" '{print $3}'`
    vg_patch_version=`valgrind --version | awk -F"[-.]" '{print $4}'`
    
    vg_num_version=$(( vg_major_version * 10000 + vg_minor_version * 100 + vg_patch_version ))
    
    if [[ $vg_num_version -lt 31300 ]]
    then
        echo "Error: please use valgrind >= 3.13.0"
        exit 1
    fi
}

set_debug_impi_env_and_timeout()
{
    export I_MPI_JOB_TIMEOUT=600
    source $I_MPI_ROOT/env/vars.sh --i_mpi_library_kind=debug
    IMPI_DEBUG_CHECK=$?
    if [ "$IMPI_DEBUG_CHECK" != "0" ]
    then
        echo "Warning: $I_MPI_ROOT/env/vars.sh --i_mpi_library_kind=debug wasn't sourced"
        exit 1
    fi
    echo "LD_LIBRARY_PATH = $LD_LIBRARY_PATH"
}

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
    local coll=$7
    echo "coll: " $coll
    local dtype=$8
    echo "dtype: " $dtype
    local reduction=${9}
    echo "reduction: " $reduction
    echo "================ENVIRONMENT=================="

    log_idx=${log_idx}+1
    base_test_log="$EXAMPLE_WORK_DIR/$dir_name/run"
    base_test_log="${base_test_log}_${transport}_${example}_b_${backend}_r_${runtime}_l_${coll}_d_${dtype}_${log_idx}"

    if [[ ${VALGRIND_SCOPE} = "regular" ]]
    then
        options=" -w 0 -i 2 -y 256,16384,32777,1048576"
    else
        options=" -w 0 -i 2 -y 256,32777,1048576"
    fi

    usm_list="none"

    buf_count=2
    if [ "${backend}" != "" ];
    then
        options="${options} --backend ${backend}"

        if [ "${backend}" == "sycl" ];
        then
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
    options="${options} --buf_count ${buf_count}"

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
    cmake .. -DCMAKE_C_COMPILER=${C_COMPILER} \
             -DCMAKE_CXX_COMPILER=${CXX_COMPILER} 2>&1 | tee ${EXAMPLE_WORK_DIR}/build_output.log
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

    reduction_list="sum"
    coll_list="allgatherv allreduce alltoall alltoallv bcast reduce reduce_scatter"
    ccl_base_env="FI_PROVIDER=tcp CCL_ATL_MPI=impi CCL_LOG_LEVEL=info I_MPI_DEBUG=2"

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
        transport_list="ofi mpi"
        runtime_list="level_zero opencl"
        if [[ ${MODE} = "gpu" ]]
        then
            dtype_list="int32 float32"
        else
            dtype_list="int32 float32 float16"
        fi
    else
        transport_list="ofi"
        runtime_list="level_zero"
        if [[ ${MODE} = "gpu" ]]
        then
            dtype_list="float32"
            coll_list="allreduce"
            transport_list="ofi"
        else
            dtype_list="float32 float16"
        fi
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
                    for dtype in $dtype_list
                    do
                        for coll in $coll_list
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
                            run_benchmark "${ccl_extra_env}" ${dir_name} ${transport} ${example} ${backend} ${runtime} ${coll} ${dtype}
                        done
                    done
                done
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
    echo_log "  C_COMPILER = clang|icc|icx|gcc|<full path to compiler>"
    echo_log "  CXX_COMPILER = clang++|icpc|icpx|g++|<full path to compiler>"
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
            echo "WARNING: Using cpu configuration of the library"
            source ${CCL_ROOT}/env/vars.sh --ccl-configuration=cpu
            MODE="cpu"
        else
            MODE="gpu"
        fi
    fi

    case $MODE in
    "cpu" )
        if [[ -z "${C_COMPILER}" ]]; then
            if [[ ",${DASHBOARD_INSTALL_TOOLS_INSTALLED}," == *",icx,"* ]]; then
                C_COMPILER=icx
            elif [[ ",${DASHBOARD_INSTALL_TOOLS_INSTALLED}," == *",icc,"* ]]; then
                C_COMPILER=icc
            else
                C_COMPILER=gcc
            fi
        fi
        if [[ -z "${CXX_COMPILER}" ]]; then
            if [[ ",${DASHBOARD_INSTALL_TOOLS_INSTALLED}," == *",icx,"* ]]; then
                CXX_COMPILER=icpx
            elif [[ ",${DASHBOARD_INSTALL_TOOLS_INSTALLED}," == *",icc,"* ]]; then
                CXX_COMPILER=icpc
            else
                CXX_COMPILER=g++
            fi
        fi
        ;;
    "gpu"|* )
        check_dpcpp
        if [ -z "${C_COMPILER}" ]
        then
            C_COMPILER=icx
        fi
        if [ -z "${CXX_COMPILER}" ]
        then
            CXX_COMPILER=dpcpp
        fi
        ;;
    esac
}

parse_arguments $@
check_mode
check_environment
set_debug_impi_env_and_timeout
check_scope
create_work_dir
build
run
