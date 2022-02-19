#!/bin/bash

BASENAME=$(basename $0 .sh)
SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`

SRC_ROOT_DIR=$(dirname $(dirname ${SCRIPT_DIR}))

CORE_COUNT=$(( $(lscpu | grep "^Socket(s):" | awk '{print $2}' ) * $(lscpu | grep "^Core(s) per socket:" | awk '{print $4}') ))
MAKE_JOB_COUNT=$(( CORE_COUNT / 3 > 4 ? CORE_COUNT / 3 : 4 ))

source ${SRC_ROOT_DIR}/scripts/utils/common.sh

pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

print_help() {
    echo ""
    echo "Usage: ${BASENAME}.sh <options>"
    echo "  --mode <mode> (default gpu)"
    echo "      cpu|gpu mode"
    echo "  --exclude-list <file>"
    echo "      Specific path to the file with excluded tests"
    echo "  --build-only"
    echo "      Enable only the build stage"
    echo "  --platform <ats|gen9|pvc>"
    echo "      Discrete GPU platform name"
    echo "  --transport <impi|mpich>"
    echo "      Transport name (default impi)"
    echo ""
    echo "Usage examples:"
    echo "  ${BASENAME}.sh --mode cpu"
    echo "  ${BASENAME}.sh --exclude-list /p/pdsd/Users/asoluyan/GitHub/oneccl/tests/reg_tests/exclude_list"
    echo "  ${BASENAME}.sh --build-only"
    echo "  ${BASENAME}.sh --mode gpu --platform gen9"
    echo ""
}

print_header() {
    echo "#"
    echo "# ${1}"
    echo "#"
}

is_exclude_test() {
    local TEST_NAME=${1}
    rc=0
    while read -r res
    do
        EXCLUDE_RULES=$(echo $res | sed "s/^${TEST_NAME}\s//g")
        for item in ${EXCLUDE_RULES}
        do
            prop="$(echo ${item} | awk -F "=" '{print $1}')"
            value="$(echo ${item} | awk -F "=" '{print $2}')"
            if [[ ${prop} = "platform" ]]
            then
                if [[ ${value} = *"${PLATFORM_HW_DISCRETE_GPU}"* || ${value} = "*" ]]
                then
                    is_exclude_platform=1
                fi
            elif [[ ${prop} = "transport" ]]
            then
                if [[ ${value} = *"${TRANSPORT}"* || ${value} = "*" ]]
                then
                    is_exclude_transport=1
                fi
            else
                echo "WARNING: unknown excluded property (${prop})"
            fi
        done
        if [[ "${is_exclude_platform}" = 1 && "${is_exclude_transport}" = 1 ]]
        then
            rc=1
            break
        fi
    done < <(grep -w ${TEST_NAME} ${EXCLUDE_LIST})
    echo ${rc}
}

set_default_values() {
    ENABLE_BUILD="yes"
    ENABLE_TESTING="yes"
    EXCLUDE_LIST="${SCRIPT_DIR}/exclude_list"

    export I_MPI_JOB_TIMEOUT=180
    export CCL_LOG_LEVEL=info
    export CCL_WORKER_COUNT=1
    export I_MPI_DEBUG=12
}

set_gen9_env() {
    if [[ ${PLATFORM_HW_DISCRETE_GPU} = "gen9" ]]
    then
        export CCL_YIELD=sched_yield
    fi
}

parse_arguments() {
    while [ $# -ne 0 ]
    do
        case $1 in
            "-h" | "--help" | "-help")
                print_help
                exit 0
                ;;
            "-mode"|"--mode")
                MODE=${2}
                shift
                ;;
            "-exclude-list"|"--exclude-list")
                export EXCLUDE_LIST=${2}
                shift
                ;;
            "-build-only"|"--build-only")
                ENABLE_BUILD="yes"
                ENABLE_TESTING="no"
                ;;
            "-platform"|"--platform")
                export PLATFORM_HW_DISCRETE_GPU=${2}
                shift
                ;;
            "-transport"|"--transport")
                export TRANSPORT=${2}
                shift
                ;;
            *)
                echo "$(basename ${0}): ERROR: unknown option (${1})"
                print_help
                exit 1
            ;;
        esac
        shift
    done

    check_mode

    if [[ ${MODE} = "gpu" ]] && [[ -z ${PLATFORM_HW_DISCRETE_GPU} ]]
    then
        export PLATFORM_HW_DISCRETE_GPU="ats"
    fi
    if [[ -z ${TRANSPORT} ]]
    then
        export TRANSPORT="impi"
    fi

    echo "-----------------------------------------------------------"
    echo "PARAMETERS"
    echo "-----------------------------------------------------------"
    echo "MODE                     = ${MODE}"
    echo "ENABLE_BUILD             = ${ENABLE_BUILD}"
    echo "ENABLE_TESTING           = ${ENABLE_TESTING}"
    echo "EXCLUDE_LIST             = ${EXCLUDE_LIST}"
    echo "PLATFORM_HW_DISCRETE_GPU = ${PLATFORM_HW_DISCRETE_GPU}"
    echo "TRANSPORT                = ${TRANSPORT}"
}

check_mode() {
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
}

define_compiler() {
    case ${MODE} in
    "cpu" )
        if [[ -z "${C_COMPILER}" ]]
        then
            if [[ ",${DASHBOARD_INSTALL_TOOLS_INSTALLED}," == *",icx,"* ]]
            then
                C_COMPILER=icx
            elif [[ ",${DASHBOARD_INSTALL_TOOLS_INSTALLED}," == *",icc,"* ]]
            then
                C_COMPILER=icc
            else
                C_COMPILER=gcc
            fi
        fi
        if [[ -z "${CXX_COMPILER}" ]]
        then
            if [[ ",${DASHBOARD_INSTALL_TOOLS_INSTALLED}," == *",icx,"* ]]
            then
                CXX_COMPILER=icpx
            elif [[ ",${DASHBOARD_INSTALL_TOOLS_INSTALLED}," == *",icc,"* ]]
            then
                CXX_COMPILER=icpc
            else
                CXX_COMPILER=g++
            fi
        fi
        ;;
    "gpu"|* )
        if [ -z "${C_COMPILER}" ]
        then
            C_COMPILER=icx
        fi
        if [ -z "${CXX_COMPILER}" ]
        then
            CXX_COMPILER=dpcpp
            COMPUTE_BACKEND="dpcpp"
        fi
        ;;
    esac
}

build() {
    if [[ ${ENABLE_BUILD} = "yes" ]]
    then
        print_header "Build tests..."
        rm -rf build
        mkdir ${SCRIPT_DIR}/build
        pushd ${SCRIPT_DIR}/build
        cmake .. -DCMAKE_C_COMPILER="${C_COMPILER}" -DCMAKE_CXX_COMPILER="${CXX_COMPILER}" -DCOMPUTE_BACKEND="${COMPUTE_BACKEND}"
        make VERBOSE=1 -j${MAKE_JOB_COUNT} install
        check_command_exit_code $? "build failed"
        popd
    fi
}

run_tests() {
    if [[ ${ENABLE_TESTING} = "yes" ]]
    then
        print_header "Run tests..."
        declare -a failed_test
        test_dirs="common"

        if [[ ${MODE} = "cpu" ]]
        then
            test_dirs="${test_dirs} cpu"
        fi

        if [[ ${MODE} = "gpu" ]]
        then
            test_dirs="${test_dirs} sycl"
        fi

        for dir in ${test_dirs}
        do
            pushd ${SCRIPT_DIR}/${dir}
            echo "DEBUG: Processing a ${dir} directory"
            tests=$(ls *.sh 2> /dev/null)
            for test in ${tests}
            do
                local name=$(basename ${test} .sh)
                is_exclude=$(is_exclude_test ${name})
                if [[ ${is_exclude} -ne 0 ]]
                then
                    echo "${test} was excluded. Skip"
                    continue
                fi
                echo "${test}"
                ./${test}
                if [[ $? -ne 0 ]]
                then
                    cat ./${name}.log
                    failed_test+=(${test})
                fi
            done
            popd
        done

        if [[ ${#failed_test[@]} > 0 ]]
        then
            echo "${#failed_test[@]} FAILED TESTS: "
            for test in "${failed_test[@]}"
            do
                echo "    ${test}"
            done
            check_command_exit_code ${#failed_test[@]} "testing failed"
        else
            echo "All tests passed"
        fi
    fi
}

set_default_values
parse_arguments $@
set_gen9_env
define_compiler
build
run_tests
