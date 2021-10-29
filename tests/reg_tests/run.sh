#!/bin/bash

BASENAME=$(basename $0 .sh)
SCRIPT_DIR=`cd $(dirname "$BASH_SOURCE") && pwd -P`

CORE_COUNT=$(( $(lscpu | grep "^Socket(s):" | awk '{print $2}' ) * $(lscpu | grep "^Core(s) per socket:" | awk '{print $4}') ))
MAKE_JOB_COUNT=$(( CORE_COUNT / 3 > 4 ? CORE_COUNT / 3 : 4 ))

pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

CheckCommandExitCode()
{
    if [ $1 -ne 0 ]; then
        echo "ERROR: ${2}" 1>&2
        exit $1
    fi
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
    echo "  --platform <ats|gen9>"
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
    res=$(grep -w ${TEST_NAME} ${EXCLUDE_LIST})
    if [[ ! -z ${res} ]]
    then
        EXCLUDE_RULES=$(echo $res | sed "s/^${TEST_NAME}\s//g")
        for item in ${EXCLUDE_RULES}
        do
            prop="$(echo ${item} | awk -F "=" '{print $1}')"
            value="$(echo ${item} | awk -F "=" '{print $2}')"
            if [[ ${prop} = "platform" ]]; then
                if [[ ${value} = *"${PLATFORM_HW_DISCRETE_GPU}"* || ${value} = "*" ]]; then is_exclude_platform=1; fi
            elif [[ ${prop} = "transport" ]]; then
                if [[ ${value} = *"${TRANSPORT}"* || ${value} = "*" ]]; then is_exclude_transport=1; fi
            else
                echo "WARNING: unknown excluded property (${prop})"
            fi
        done
    fi
    if [[ "${is_exclude_platform}" = 1 && "${is_exclude_transport}" = 1 ]]
    then
        rc=1
    fi
    echo ${rc}
}

set_default_values() {
    ENABLE_BUILD="yes"
    ENABLE_TESTING="yes"
    EXCLUDE_LIST="${SCRIPT_DIR}/exclude_list"
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
                PLATFORM_HW_DISCRETE_GPU=${2}
                shift
                ;;
            "-transport"|"--transport")
                TRANSPORT=${2}
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
        PLATFORM_HW_DISCRETE_GPU="ats"
    fi
    if [[ -z ${TRANSPORT} ]]
    then
        TRANSPORT="impi"
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
        if [ -z "${C_COMPILER}" ]; then 
            C_COMPILER=icx
        fi
        if [ -z "${CXX_COMPILER}" ]; then
            CXX_COMPILER=dpcpp
            COMPUTE_BACKEND="dpcpp_level_zero"
        fi
        ;;
    esac
}

build() {
    if [[ ${ENABLE_BUILD} = "yes" ]]; then
        print_header "Build tests..."
        rm -rf build
        mkdir ${SCRIPT_DIR}/build
        pushd ${SCRIPT_DIR}/build
        cmake .. -DCMAKE_C_COMPILER="${C_COMPILER}" -DCMAKE_CXX_COMPILER="${CXX_COMPILER}" -DCOMPUTE_BACKEND="${COMPUTE_BACKEND}"
        make VERBOSE=1 -j${MAKE_JOB_COUNT} install
        CheckCommandExitCode $? "build failed"
        popd
    fi
}

run_tests() {
    if [[ ${ENABLE_TESTING} = "yes" ]]; then
        print_header "Run tests..."
        declare -a failed_test
        test_dirs="common"

        if [[ ${MODE} = "cpu" ]]; then
            test_dirs="${test_dirs} cpu"
        fi

        if [[ ${MODE} = "gpu" ]]; then
            test_dirs="${test_dirs} sycl"
        fi

        for dir in ${test_dirs}; do
            pushd ${SCRIPT_DIR}/${dir}
            echo "DEBUG: Processing a ${dir} directory"
            tests=$(ls *.sh 2> /dev/null)
            for test in ${tests}; do
                is_exclude=$(is_exclude_test $(basename ${test} .sh))
                if [[ ${is_exclude} -ne 0 ]]; then
                    echo "${test} was excluded. Skip"
                    continue
                fi
                echo "${test}"
                ./${test}
                if [[ $? -ne 0 ]]; then
                    failed_test+=(${test})
                fi
            done
            popd
        done

        if [[ ${#failed_test[@]} > 0 ]]; then
            echo "${#failed_test[@]} FAILED TESTS: "
            for test in "${failed_test[@]}"; do
                echo "    ${test}"
            done
            CheckCommandExitCode ${#failed_test[@]} "testing failed"
        else
            echo "All tests passed"
        fi
    fi
}

set_default_values
parse_arguments $@
define_compiler
build
run_tests
