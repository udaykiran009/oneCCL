function set_ats_mpich_ofi_env()
{
    if [[ -z ${CURRENT_WORK_DIR} ]]
    then
        CURRENT_WORK_DIR=`cd ${SCRIPT_DIR}/../../../ && pwd -P`
        echo "CURRENT_WORK_DIR: ${CURRENT_WORK_DIR}"
    fi
    if [[ ! -d "${CURRENT_WORK_DIR}/scripts/mpich_ofi/install" ]]
    then
        ${CURRENT_WORK_DIR}/scripts/mpich_ofi/mpich_ofi.sh
    fi
    export MPICH_LIBFABRIC_PATH="/home/sys_csr1/software/libfabric/sockets-dynamic/lib"
    MPICH_OFI_INSTALL_PATH=$( cd ${CURRENT_WORK_DIR}/scripts/mpich_ofi/install/usr/mpi/mpich-ofi*/ && pwd -P )
    echo "MPICH_OFI_INSTALL_PATH: ${MPICH_OFI_INSTALL_PATH}"
    export PATH="${MPICH_OFI_INSTALL_PATH}/bin:${PATH}"
    export LD_LIBRARY_PATH="${MPICH_OFI_INSTALL_PATH}/lib:${MPICH_LIBFABRIC_PATH}:${LD_LIBRARY_PATH}"
    unset FI_PROVIDER_PATH
}
