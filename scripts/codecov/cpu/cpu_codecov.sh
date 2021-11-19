#!/bin/bash

CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo "ERROR: ${2}" 1>&2
        exit $1
    fi
}

set_default_values() {
    ARTEFACT_DIR="${ARTEFACT_ROOT_DIR}/${MLSL_BUILDER_NAME}/${MLSL_BUILD_ID}"
    COMPFILE_DIR="${ARTEFACT_DIR}/scripts/codecov/cpu/compfiles/"
    REPORT_FOLDER="/p/pdsd/scratch/Uploads/RegularTesting/CCL/code-coverage-reports"
    PATH_TO_DROP=${REPORT_FOLDER}/$(date +'%Y-%m-%d')
    ICC_BUNDLE_ROOT="/nfs/inn/proj/mpi/pdsd/opt/EM64T-LIN/parallel_studio/parallel_studio_xe_2020.0.088/compilers_and_libraries_2020/"
}

set_environment() {
    source ${ARTEFACT_DIR}/_package/eng/env/vars.sh --ccl-configuration=cpu
    source ${ICC_BUNDLE_ROOT}/linux/bin/compilervars.sh intel64

    export DASHBOARD_INSTALL_TOOLS_INSTALLED=icc
    export BUILD_COMPILER_TYPE=intel
}

run_tests() {
    ${ARTEFACT_DIR}/scripts/jenkins/sanity.sh -compatibility_tests
    CheckCommandExitCode $? "compatibility testing failed"

    # Exclude size_checker - extra size of lib due to codecov extra symbols
    echo "size_checker  platform=* transport=*" >> ${ARTEFACT_DIR}/tests/reg_tests/exclude_list
    ${ARTEFACT_DIR}/scripts/jenkins/sanity.sh -reg_tests
    CheckCommandExitCode $? "regression testing failed"

    for mode in mpi ofi mpi_adjust ofi_adjust priority_mode dynamic_pointer_mode fusion_mode unordered_coll_mode
    do
        BUILD_COMPILER_TYPE=intel runtime=${mode} ${ARTEFACT_DIR}/scripts/jenkins/sanity.sh -functional_tests
        CheckCommandExitCode $? "functional testing failed"
    done
}

collect_results() {
    #Collect all folders from scr
    pushd ${ARTEFACT_DIR}/src
    CODE_FOLDERS=""
    for dir_name in *
    do
        if [ -d "$dir_name" ]; then
            CODE_FOLDERS=${CODE_FOLDERS}" "${dir_name}
        fi
    done
    pushd ${ARTEFACT_DIR}/build/src
    profmerge -prof_dpi pgopti_func.dpi

    mkdir -p ${ARTEFACT_DIR}/Coverage_results/coverage-overall/
    codecov -comp ${COMPFILE_DIR}/compfile-overall -prj ccl -spi pgopti.spi -dpi pgopti_func.dpi -xmlbcvrgfull codecov.xml

    mv CodeCoverage/ ${ARTEFACT_DIR}/Coverage_results/coverage-overall/
    mv CODE_COVERAGE.HTML ${ARTEFACT_DIR}/Coverage_results/coverage-overall/

    for folder_name in ${CODE_FOLDERS}
    do
        mkdir -p ${ARTEFACT_DIR}/Coverage_results/coverage-${folder_name}/

        #Add folder for code coverage
        echo "src/"${folder_name}"/" > $COMPFILE_DIR/compfile-${folder_name}
        #Exclude folders
        echo "~usr" >> $COMPFILE_DIR/compfile-${folder_name}
        echo "~log.hpp" >> $COMPFILE_DIR/compfile-${folder_name}

        echo "Collecting ${folder_name} coverage..."
        
        codecov -prj ccl -comp $COMPFILE_DIR/compfile-${folder_name} -spi pgopti.spi -dpi pgopti_func.dpi

        mv CodeCoverage/ ${ARTEFACT_DIR}/Coverage_results/coverage-${folder_name}/
        mv CODE_COVERAGE.HTML ${ARTEFACT_DIR}/Coverage_results/coverage-${folder_name}/
    done

    rm -rf *.dyn
    pushd ${ARTEFACT_DIR}/Coverage_results/
    mv ${ARTEFACT_DIR}/build/src/codecov.xml .

    mkdir -p ${PATH_TO_DROP}
    cp -r ${ARTEFACT_DIR}/Coverage_results ${PATH_TO_DROP}

    rm -rf ${REPORT_FOLDER}/last
    ln -s ${PATH_TO_DROP} ${REPORT_FOLDER}/last
}

set_default_values
set_environment
run_tests
collect_results
