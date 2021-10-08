function check_impi() {
    if [[ -z "${I_MPI_ROOT}" ]] ; then
        echo "Error: \$I_MPI_ROOT is not set. Please source vars.sh"
        exit 1
    fi
}

function check_ccl() {
    if [[ -z "${CCL_ROOT}" ]]; then
        echo "Error: \$CCL_ROOT is not set. Please source vars.sh"
        exit 1
    fi
}

function CheckCommandExitCode() {
    if [ ${1} -ne 0 ]
    then
        echo "ERROR: ${2}"
        exit ${1}
    fi
}
