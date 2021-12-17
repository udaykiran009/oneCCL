#!/bin/bash -xe

echo "DEBUG: WORKSPACE = ${WORKSPACE}"
echo "DEBUG: TESTING_ENVIRONMENT = ${TESTING_ENVIRONMENT}"

ARTEFACT_DIR=${ARTEFACT_ROOT_DIR}/${BUILDER_NAME}/${MLSL_BUILD_ID}/

groups
groups | wc -w

CheckCommandExitCode() {
    if [ $1 -ne 0 ]; then
        echo "ERROR: ${2}" 1>&2
        exit $1
    fi
}

which cmake 
which make && make --version

if [ $build_type == "debug" ]
then
    export BUILD_OPTIONS="-build-deb-conf"
fi    
which gcc && gcc -v

source ${WORKSPACE}/scripts/jenkins/fake_container.sh

if [ $compiler == "ccl_build_gnu_4.8.5" ] 
then
(
    export compute_backend=
    export compiler="gnu"
    run_in_fake_container /build/ccl/scripts/build.sh --conf --build-cpu $BUILD_OPTIONS
    CheckCommandExitCode $? "build with gnu_4.8.5 compiler failed"
    (cd ${WORKSPACE} && tar cfz ${ARTEFACT_DIR}/${BUILD_FOLDER}_$build_type.tgz build)
)
elif [ $compiler == "ccl_build_gnu_10.2.0" ]
then
(
    export compute_backend= 
    export compiler="gnu"
    # w/a for Ubuntu 20.4 and GCC 10.2
    export LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/:$LIBRARY_PATH
    export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/:$LD_LIBRARY_PATH
    export GNU_BUNDLE_ROOT="/p/pdsd/opt/EM64T-LIN/compilers/gnu/gcc-10.2.0/bin"
    run_in_fake_container /build/ccl/scripts/build.sh --conf --build-cpu $BUILD_OPTIONS
    CheckCommandExitCode $? "build with gnu compiler failed"
    (cd ${WORKSPACE} && tar cfz ${ARTEFACT_DIR}/${BUILD_FOLDER}_$build_type.tgz build)
)
elif [ $compiler == "ccl_build_dpcpp_l0" ]
then
    export compute_backend="dpcpp"
    run_in_fake_container /build/ccl/scripts/build.sh --conf --build-gpu $BUILD_OPTIONS
    CheckCommandExitCode $? "build with dpcpp compiler failed"
    (cd ${WORKSPACE} && tar cfz ${ARTEFACT_DIR}/build_gpu_$build_type.tgz build_gpu)
elif [ $compiler == "ccl_build_dpcpp" ]
then
(
    export compute_backend="dpcpp"
    export compiler="dpcpp"
    run_in_fake_container /build/ccl/scripts/build.sh --conf --build-gpu $BUILD_OPTIONS
    CheckCommandExitCode $? "build with dpcpp compiler failed"
    (cd ${WORKSPACE} && tar cfz ${ARTEFACT_DIR}/${BUILD_FOLDER}_$build_type.tgz build)
)
elif [ $compiler == "ccl_build_intel" ]  
then
(
    export compiler="intel"
    run_in_fake_container /build/ccl/scripts/build.sh --conf --build-cpu $BUILD_OPTIONS
    CheckCommandExitCode $? "build with intel compiler failed"
    (cd ${WORKSPACE} && tar cfz ${ARTEFACT_DIR}/build_cpu_$build_type.tgz build)
)
elif [ $compiler == "ccl_build_icx" ]
then
(
    export compiler="icx"
    run_in_fake_container /build/ccl/scripts/build.sh --conf --build-cpu $BUILD_OPTIONS
    CheckCommandExitCode $? "build with icx compiler failed"
    (cd ${WORKSPACE} && tar cfz ${ARTEFACT_DIR}/build_cpu_icx_$build_type.tgz build)
)
fi
