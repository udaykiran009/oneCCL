function set_ats_mpich_ofi_env()
{
    if [[ -z ${CURRENT_WORK_DIR} ]]
    then
        CURRENT_WORK_DIR=`cd ${SCRIPT_DIR}/../../../ && pwd -P`
        echo "CURRENT_WORK_DIR: ${CURRENT_WORK_DIR}"
    fi
    ${CURRENT_WORK_DIR}/scripts/mpich_ofi/mpich_ofi.sh
    MPICH_OFI_INSTALL_PATH=$( cd ${CURRENT_WORK_DIR}/scripts/mpich_ofi/install/usr/mpi/mpich-ofi*/ && pwd -P )
    echo "MPICH_OFI_INSTALL_PATH: ${MPICH_OFI_INSTALL_PATH}"
    export PATH="${MPICH_OFI_INSTALL_PATH}/bin:${PATH}"
    export LD_LIBRARY_PATH="${MPICH_OFI_INSTALL_PATH}/lib:${LD_LIBRARY_PATH}"
    unset FI_PROVIDER_PATH
    mkdir -p ${CURRENT_WORK_DIR}/scripts/mpich_ofi/prov
    echo "I_MPI_ROOT: ${I_MPI_ROOT}" 
    cp ${I_MPI_ROOT}/libfabric/lib/prov/libpsm3-fi.so ${CURRENT_WORK_DIR}/scripts/mpich_ofi/prov
    export FI_PROVIDER_PATH="${CURRENT_WORK_DIR}/scripts/mpich_ofi/prov"
}
