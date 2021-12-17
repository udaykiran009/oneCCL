#!/bin/bash

SCRIPT_DIR=$(cd $(dirname "$BASH_SOURCE") && pwd -P)
source ${SCRIPT_DIR}/../../scripts/jenkins/fake_container.sh
source ${SCRIPT_DIR}/../../scripts/utils/common.sh

runtime_path=$(pwd)
export CFLAGS="-fPIC"
export CXXFLAGS="-fPIC"
export FAKE_WORKSPACE_PATH="/build/hwloc"
HWLOC_TAG="hwloc-2.7.0"

git clone --depth 1 --branch ${HWLOC_TAG} https://github.com/open-mpi/hwloc.git
cd ./hwloc
install_path=$(pwd)/_install

cat > build_hwloc.sh << EOF
#!/bin/bash
cd ${FAKE_WORKSPACE_PATH}
./autogen.sh
./configure --prefix=${FAKE_WORKSPACE_PATH}/_install \
    --disable-netloc \
    --disable-opencl \
    --disable-cuda \
    --disable-nvml \
    --disable-rsmi \
    --disable-levelzero \
    --disable-gl \
    --disable-libxml2 \
    --enable-static \
    --disable-libudev
make -j all && make install
EOF

chmod +x build_hwloc.sh
run_in_fake_container ${FAKE_WORKSPACE_PATH}/build_hwloc.sh
check_command_exit_code $? "build hwloc failed"

cp -rf ${install_path}/include ${runtime_path}
cp -f ${install_path}/lib/libhwloc.a ${runtime_path}/lib
