function set_ats_mpich_ofi_env()
{
    ${CURRENT_WORK_DIR}/scripts/mpich_ofi/mpich_ofi.sh
    MPICH_OFI_INSTALL_PATH=$( cd ${CURRENT_WORK_DIR}/scripts/mpich_ofi/install/usr/mpi/mpich-ofi*/ && pwd -P )
    export LD_LIBRARY_PATH="${MPICH_OFI_INSTALL_PATH}/lib:${LD_LIBRARY_PATH}"
    unset FI_PROVIDER_PATH
    mkdir -p ${CURRENT_WORK_DIR}/scripts/mpich_ofi/prov
    cp ${I_MPI_ROOT}/libfabric/lib/prov/libpsm3-fi.so ${CURRENT_WORK_DIR}/scripts/mpich_ofi/prov
    export FI_PROVIDER_PATH="${CURRENT_WORK_DIR}/scripts/mpich_ofi/prov"
}
