#!/bin/bash

set_default_values() {
    BASENAME=`basename $0 .sh`
    SCRIPT_DIR=$(cd $(dirname "$BASH_SOURCE") && pwd -P)
    SRC_ROOT_DIR=$(realpath ${SCRIPT_DIR}/../../)
    IMPI_PATH="/p/pdsd/scratch/Uploads/CCL_oneAPI/mpi_oneapi/last/mpi/latest"
    DPCPP_PATH="/p/pdsd/scratch/Uploads/CCL_oneAPI/compiler/l_compiler_p_2022.0.0.071/"
    PROFILE_DIR="${SCRIPT_DIR}/_profile_dir"

    export SYCL_BUNDLE_ROOT=${DPCPP_PATH}/compiler/latest/linux/

    ENABLE_BUILD="no"
    ENABLE_TESTING="no"
    ENABLE_REPORT="no"
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
    echo "  --build"
    echo "      Enable build oneCCL with code coverage flags"
    echo "  --test"
    echo "      Enable code coverage testing"
    echo ""
    echo "Usage examples:"
    echo "  ${BASENAME}.sh --build"
    echo "  ${BASENAME}.sh --test"
    echo "  ${BASENAME}.sh --report"
    echo ""
}

print_header() {
    echo "#"
    echo "# ${1}"
    echo "#"
}

parse_arguments() {
    while [ $# -ne 0 ]
    do
        case $1 in
            "-h" | "--help" | "-help")
                print_help
                exit 0
                ;;
            "-build"|"--build")
                ENABLE_BUILD="yes"
                ;;
            "-test"|"--test")
                ENABLE_TESTING="yes"
                ;;
            "-report"|"--report")
                ENABLE_REPORT="yes"
                ;;   
            *)
                echo "$(basename $0): ERROR: unknown option ($1)"
                print_help
                exit 1
            ;;
        esac
        shift
    done

    echo "-----------------------------------------------------------"
    echo "PARAMETERS"
    echo "-----------------------------------------------------------"
    echo "ENABLE_BUILD       = ${ENABLE_BUILD}"
    echo "ENABLE_TESTING     = ${ENABLE_TESTING}"
    echo "ENABLE_REPORT      = ${ENABLE_REPORT}"

}

check_dpcpp()
{
    which dpcpp
    COMPILER_INSTALL_CHECK=$?
    if [ "${COMPILER_INSTALL_CHECK}" != "0" ]
    then
        echo "Warning: DPCPP compiler wasn't found, Will be used default: ${DPCPP_PATH}"
        source ${DPCPP_PATH}/setvars.sh
    fi
}

check_impi() {
    if [[ -z "${I_MPI_ROOT}" ]]
    then
        echo "Error: \$I_MPI_ROOT is not set. Will be used default: ${IMPI_PATH}"
        source ${IMPI_PATH}/env/vars.sh
    fi
}

check_ccl() {
    if [[ -z "${CCL_ROOT}" ]]
    then
        if [[ -d ${SRC_ROOT_DIR}/build_gpu/_install ]]
        then
            source ${SRC_ROOT_DIR}/build_gpu/_install/env/vars.sh
        else
            echo "Error: The compiled library could not be found or \$CCL_ROOT is not set."
            echo "       Please use the '--build' option or source vars.sh."
            exit 1
        fi
    fi
}

set_environment() {
    check_dpcpp
    check_impi

    export BUILD_COMPILER_TYPE="dpcpp"
    export node_label="ccl_test_gen9"
}

create_report() {
    if [[ ${ENABLE_REPORT} = "yes" ]]
    then
        check_ccl
        pushd ${SRC_ROOT_DIR}/src
        CODE_FOLDERS=""
        for DIR_NAME in *
        do
            if [[ -d "${DIR_NAME}" ]]
            then
                grep "${DIR_NAME}" ${SCRIPT_DIR}/exclude_list
                if [[ $? = 0 ]]
                then
                    continue
                fi
                CODE_FOLDERS="${CODE_FOLDERS} ${DIR_NAME}"
            fi
        done
        CODE_FOLDERS="${CODE_FOLDERS}"

        mkdir -p ${SCRIPT_DIR}/compfiles
        for folder_name in ${CODE_FOLDERS}
        do
            echo "src/${folder_name}/" > ${SCRIPT_DIR}/compfiles/compfile-${folder_name}
            echo "src/${folder_name}/" >> ${SCRIPT_DIR}/compfiles/compfile-overall
        done

        for compfile in $(ls ${SCRIPT_DIR}/compfiles/)
        do
            print_header "Creating report for ${compfile}..."
            get_sources ${compfile}

            llvm-cov show ${CCL_ROOT}/lib/libccl.so -instr-profile=${PROFILE_DIR}/code.profdata -use-color --format html ${SOURCES} > ${PROFILE_DIR}/coverage_${compfile}.html
            CheckCommandExitCode $? "Creating html report failed"

            llvm-cov report ${CCL_ROOT}/lib/libccl.so -instr-profile=${PROFILE_DIR}/code.profdata ${SOURCES} > ${PROFILE_DIR}/report_${compfile}.log
            CheckCommandExitCode $? "Creating text report failed"

            # Counting covered files
            uncoverage_files=$(grep -E -i -w '^.*\.(cpp|hpp|c|h)(.*\s0\.00%){3,3}' ${PROFILE_DIR}/report_${compfile}.log | wc -l)
            total_files=$(grep -c -e '^.*.\(cpp\|c\|hpp\|h\)' ${PROFILE_DIR}/report_${compfile}.log)
            echo "total    uncvrg    cvrg%" > ${PROFILE_DIR}/report_files_${compfile}.log
            echo "${total_files}    ${uncoverage_files}    $(bc <<< "scale=2; (1-${uncoverage_files}/${total_files})*100")%" >> ${PROFILE_DIR}/report_files_${compfile}.log

            # Print metrics
            echo "Files: $(tail -1 ${PROFILE_DIR}/report_files_${compfile}.log | awk '{print $1, $2, $3}')"
            echo "Regions: $(tail -1 ${PROFILE_DIR}/report_${compfile}.log | awk '{print $2, $3, $4}')"
            echo "Functions: $(tail -1 ${PROFILE_DIR}/report_${compfile}.log | awk '{print $5, $6, $7}')"
            echo "Lines: $(tail -1 ${PROFILE_DIR}/report_${compfile}.log | awk '{print $8, $9, $10}')"
            echo "Branches: $(tail -1 ${PROFILE_DIR}/report_${compfile}.log | awk '{print $11, $12, $13}')"
            print_header "Creating for ${compfile}...DONE"
        done
    fi
}

get_sources() {
    mkdir -p ${SCRIPT_DIR}/_tmp
    touch ${SCRIPT_DIR}/_tmp/include_files
    compfile=${1}

    for dir in $(cat ${SCRIPT_DIR}/compfiles/${compfile}); do
        find ${SRC_ROOT_DIR}/${dir} -type f -regex '.*\.\(c\|cpp\|h\|hpp\)$' -print | sort >> ${SCRIPT_DIR}/_tmp/include_files
    done

    SOURCES=$(cat ${SCRIPT_DIR}/_tmp/include_files)
    
    rm -r ${SCRIPT_DIR}/_tmp
}

build_oneccl() {
    if [[ ${ENABLE_BUILD} = "yes" ]]
    then
        print_header "Building..."

        ENABLE_DEBUG_BUILD="yes" ENABLE_CODECOV="yes" ${SRC_ROOT_DIR}/scripts/build.sh --conf --build-gpu
        CheckCommandExitCode $? "Building failed"

        # After use build.sh the env directory will contain the oneapi version vars.sh. 
        # This is incorrect for the current case, so the public version of the vars.sh script is copied.
        cp ${SRC_ROOT_DIR}/ccl_public/vars.sh.in ${SRC_ROOT_DIR}/build_gpu/_install/env/vars.sh

        print_header "Building...DONE"
    fi
}

run_tests() {
    if [[ ${ENABLE_TESTING} = "yes" ]]
    then
        print_header "Run testing..."
        check_ccl
        
        # Run examples
        LLVM_PROFILE_FILE="${PROFILE_DIR}/regular-%p.profraw" ${SRC_ROOT_DIR}/scripts/jenkins/sanity.sh -regular_tests
        CheckCommandExitCode $? "regular tests failed"

        llvm-profdata merge -output=${PROFILE_DIR}/code_regular.profdata ${PROFILE_DIR}/regular-*.profraw
        rm -f ${PROFILE_DIR}/regular-*.profraw

        # Run functional tests
        for mode in mpi ofi mpi_adjust ofi_adjust priority_mode dynamic_pointer_mode fusion_mode unordered_coll_mode
        do
            LLVM_PROFILE_FILE="${PROFILE_DIR}/func_${mode}-%p.profraw" runtime=${mode} ${SRC_ROOT_DIR}/scripts/jenkins/sanity.sh -functional_tests
            CheckCommandExitCode $? "functional testing failed"

            llvm-profdata merge -output=${PROFILE_DIR}/code_func_${mode}.profdata ${PROFILE_DIR}/func_*.profraw
            rm -f ${PROFILE_DIR}/func_*.profraw
        done
        llvm-profdata merge -output=${PROFILE_DIR}/code_func.profdata ${PROFILE_DIR}/code_func_*.profdata

        # Run regression tests
        echo "size_checker  platform=* transport=*" >> ${SRC_ROOT_DIR}/tests/reg_tests/exclude_list
        echo "pkgconfig_support_checker  platform=* transport=*" >> ${SRC_ROOT_DIR}/tests/reg_tests/exclude_list
        LLVM_PROFILE_FILE="${PROFILE_DIR}/regression-%p.profraw" ${SRC_ROOT_DIR}/scripts/jenkins/sanity.sh -reg_tests
        CheckCommandExitCode $? "regression testing failed"

        llvm-profdata merge -output=${PROFILE_DIR}/code_regression.profdata ${PROFILE_DIR}/regression-*.profraw
        rm -f ${PROFILE_DIR}/regression-*.profraw

        llvm-profdata merge -output=${PROFILE_DIR}/code.profdata ${PROFILE_DIR}/code_func.profdata ${PROFILE_DIR}/code_regular.profdata ${PROFILE_DIR}/code_regression.profdata

        print_header "Run testing...DONE"
    fi
}

set_default_values
parse_arguments "$@"
set_environment
build_oneccl
run_tests
create_report

