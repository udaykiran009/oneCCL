#!/bin/bash
set -o pipefail

readonly ARGS=( "$@" )
readonly BASENAME=$(basename $0)

readonly EXIT_FAILURE=1

print_help()
{
    echo "Usage: ${BASENAME} --name NAME --old PATH --new PATH --report PATH"
    echo "  --name NAME             any name of the library"
    echo "  --old PATH              path to the old 'build' dir"
    echo "  --new PATH              path to the new 'build' dir"
    echo "  --report-dir PATH       path to the directory where the report will be placed"
    echo "  --dump-dir PATH         (optional) path to the directory where dumps will be placed"
    echo "                          Default: system temp dir"
    echo "  --build-type TYPE       (optional) specify the build type."
    echo "                          Used ONLY if compared builds have passed the post build step."
    echo "                          Example: cpu_icc or cpu_gpu_dpcpp"
    echo
    echo "WARNING: Debug CCL build is needed:"
    echo "  build.sh --build-cpu --build-gpu --build-deb-conf"
    echo "or"
    echo "  build.sh --build-cpu --build-gpu --post-build --build-deb-conf"
    echo
    echo "Usage examples:"
    echo "  1) Comparison of two builds:"
    echo "     ${BASENAME} --name libccl_cpu --old ./mlsl_old/build --new ./mlsl_new/build --report-dir ./report"
    echo "     or"
    echo "     ${BASENAME} --name libccl_gpu --old ./mlsl_old/build_gpu --new ./mlsl_new/build_gpu --report-dir ./report"
    echo
    echo "  2) Comparison of two builds after the post build step:"
    echo "     ${BASENAME} --name libccl_cpu --old ./mlsl_old/build --new ./mlsl_new/build --report-dir ./report --build-type cpu_icc"
    echo "     or"
    echo "     ${BASENAME} --name libccl_gpu --old ./mlsl_old/build --new ./mlsl_new/build --report-dir ./report --build-type cpu_gpu_dpcpp"
}

parse_arguments()
{
    while [ $# -ne 0 ]
    do
        case ${1} in
            "-h" | "--help" | "-help")                  print_help ; exit 0 ;;
            "--name" | "-name")                         LIBRARY_NAME=${2} ; shift ;;
            "--old" | "-old")                           OLD_BUILD="$(realpath ${2})" ; shift ;;
            "--new" | "-new")                           NEW_BUILD="$(realpath ${2})" ; shift ;;
            "--report-dir" | "-report-dir")             REPORT_DIR="$(realpath ${2})" ; shift ;;
            "--dump-dir" | "-dump-dir")                 DUMP_DIR="$(realpath ${2})" ; shift ;;
            "--build-type" | "-build-type")             BUILD_TYPE=${2} ; shift ;;
            *)
            echo "${BASENAME}: ERROR: unknown option (${1})"
            print_help
            exit ${EXIT_FAILURE}
            ;;
        esac
        shift
    done

    if [[ -z "${LIBRARY_NAME}" ]] || [[ -z "${OLD_BUILD}" ]] || [[ -z "${NEW_BUILD}" ]] || [[ -z "${REPORT_DIR}" ]]; then
        echo "Incorrect options"
        print_help
        exit ${EXIT_FAILURE}
    fi

    echo "Parameters:"
    [[ ! -z "${LIBRARY_NAME}" ]] && echo "LIBRARY_NAME: ${LIBRARY_NAME}"
    [[ ! -z "${OLD_BUILD}" ]] && echo "OLD_BUILD: ${OLD_BUILD}"
    [[ ! -z "${NEW_BUILD}" ]] && echo "NEW_BUILD: ${NEW_BUILD}"
    [[ ! -z "${REPORT_DIR}" ]] && echo "REPORT_DIR: ${REPORT_DIR}"
    [[ ! -z "${DUMP_DIR}" ]] && echo "DUMP_DIR: ${DUMP_DIR}"
    [[ ! -z "${BUILD_TYPE}" ]] && echo "BUILD_TYPE: ${BUILD_TYPE}"
    echo
}

check_command()
{
    local cmd=${1}
    command -v ${cmd} >/dev/null 2>&1 || { echo >&2 "Error: ${cmd} wasn't found. Aborting."; exit ${EXIT_FAILURE}; }
}

set_env()
{
    source /p/pdsd/scratch/Uploads/CCL_oneAPI/compiler/last/compiler/latest/env/vars.sh intel64
    check_command clang++

    UTILS_DIR=/nfs/inn/disks/nn-ssg_tcar_mpi_2Tb_unix/Uploads/CCL/jenkins-utils/abi
    export PATH=${UTILS_DIR}/abi-compliance-checker/bin:$PATH
    check_command abi-compliance-checker

    export PATH=${UTILS_DIR}/abi-dumper/bin:$PATH
    check_command abi-dumper

    export PATH=${UTILS_DIR}/ctags/bin:$PATH
    check_command ctags

    export PATH=${UTILS_DIR}/elfutils/bin:$PATH
    export LD_LIBRARY_PATH=${UTILS_DIR}/elfutils/lib:$LD_LIBRARY_PATH
    export C_INCLUDE_PATH=${UTILS_DIR}/elfutils/include:$C_INCLUDE_PATH
    check_command readelf

    export PATH=${UTILS_DIR}/vtable-dumper/bin:$PATH
    check_command vtable-dumper
}

declare -i dump_version=0

generate_dump()
{
    local build_dir=${1}
    local output=${2}
    ((dump_version++))

    local so_name="libccl.so"
    local so_dir="${build_dir}/lib/${BUILD_TYPE}"

    if [[ ! -f ${so_dir}/${so_name} ]]; then
        so_name="libccl.so.1.0"
    fi

    echo
    echo "Dumping ${so_dir}/${so_name} ..."
    abi-dumper ${so_dir}/${so_name} -lambda -lver ${dump_version} -o ${output} \
                -ld-library-path ${LD_LIBRARY_PATH} \
                -skip-cxx \
                -public-headers ${build_dir}/include/${BUILD_TYPE}/ \
                -mixed-headers \
                -bin-only \
                -all-types
}

print_abi_breakages()
{
    local name=${1}
    local abi_affected="${REPORT_DIR}/${name}/abi_affected.txt"
    local src_affected="${REPORT_DIR}/${name}/src_affected.txt"

    if [[ $(stat -c%s "${abi_affected}") -gt 0 ]]; then
        echo
        echo "ABI breakages detected:"
        cat ${abi_affected} | c++filt
        echo "List saved to file ${abi_affected}"
    fi
    if [[ $(stat -c%s "${src_affected}") -gt 0 ]]; then
        echo
        echo "API breakages detected:"
        cat ${src_affected} | c++filt
        echo "List saved to file ${src_affected}"
    fi
    echo
}

compare_dumps()
{
    local name=${1}
    local old_dump=${2}
    local new_dump=${3}
    local output=${REPORT_DIR}/${name}/abi_dumper_compare_output.txt

    mkdir -p ${REPORT_DIR}/${name}

    echo
    echo "Comparing dumps ..."
    abi-dumper --compare ${old_dump} ${new_dump} > ${output}
    cat ${output}
    local symbols_changed=$(cat ${output} | grep -- 'Removed\|Modified' | wc -l)
    if [[ ${symbols_changed} != 0 ]]; then
        echo "Symbols have been changed:"
        cat ${output} | grep -- 'Removed\|Modified' | c++filt
        #ST_ERROR=1
    fi
}

check_abi()
{
    local name=${1}
    local old_dump=${2}
    local new_dump=${3}
    local output_dir=${REPORT_DIR}/${name}

    mkdir -p ${output_dir}

    echo
    echo "ABI checking ..."
    abi-compliance-checker  -l ${name} \
                            -old ${old_dump} \
                            -new ${new_dump} \
                            -ext \
                            -strict \
                            -list-affected \
                            -report-path ${output_dir}/compat_report.html
    if [[ $? -ne 0 ]]; then
        print_abi_breakages ${name}
        ST_ERROR=1
    fi
    cat ${output_dir}/abi_affected.txt | c++filt > ${output_dir}/abi_affected_demangled.txt
    cat ${output_dir}/src_affected.txt | c++filt > ${output_dir}/src_affected_demangled.txt
    echo
    echo "Report dir: ${output_dir}"
    echo
}

run()
{
    if [[ -z "${DUMP_DIR}" ]]; then
        readonly DUMPS_TMP_DIR=$(mktemp -d -t ABI-XXXXXXXXXX) # temp dir
        echo "Temp dir: ${DUMPS_TMP_DIR}"
    else
        readonly DUMPS_TMP_DIR=${DUMP_DIR}
    fi
    readonly OLD_DUMP=${DUMPS_TMP_DIR}/old.dump
    readonly NEW_DUMP=${DUMPS_TMP_DIR}/new.dump
    generate_dump ${OLD_BUILD}/_install/ ${OLD_DUMP}
    generate_dump ${NEW_BUILD}/_install/ ${NEW_DUMP}

    compare_dumps ${LIBRARY_NAME} ${OLD_DUMP} ${NEW_DUMP}
    check_abi ${LIBRARY_NAME} ${OLD_DUMP} ${NEW_DUMP}
}

clean()
{
    echo "Deleting temporary files ..."
    if [[ -z "${DUMP_DIR}" ]] && [[ ! -z "${DUMPS_TMP_DIR}" ]]; then
        echo "Delete ${DUMPS_TMP_DIR}"
        rm -r "${DUMPS_TMP_DIR}"
    fi
    echo
}

parse_arguments $ARGS
set_env
run
clean

if [[ ! -z "${ST_ERROR}" ]] && [[ ${ST_ERROR} != 0 ]]; then
    echo "ABI is changed !!!"
    exit ${EXIT_FAILURE}
else
    echo "No changes"
    echo "Ok."
fi
