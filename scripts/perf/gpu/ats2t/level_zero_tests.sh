#! /bin/bash

WORK_DIR="/home/gta/mshiryae"

LEVEL_ZERO_TESTS_URL="https://gitlab.devtools.intel.com/ict/ccl-team/level-zero-tests.git"
LEVEL_ZERO_TESTS_BRANCH="ccl"

L0_TESTS_URL="https://gitlab.devtools.intel.com/zdworkin/l0_tests.git"
L0_TESTS_BRANCH="master"

sudo apt-get install libboost-all-dev

mkdir -p ${WORK_DIR}

# bench from L0 team
cd $WORD_DIR
git clone -b ${LEVEL_ZERO_TESTS_BRANCH} --single-branch ${LEVEL_ZERO_TESTS_URL}
cd level-zero-tests
mkdir build
cd build
cmake -D GROUP=/perf_tests -D CMAKE_INSTALL_PREFIX=$PWD/../out/perf_tests ..
cmake --build . --config Release --target install
cd ${WORK_DIR}/level-zero-tests/out/perf_tests
./ze_bandwidth -t d2h -i 100 | tee d2h.log
./ze_bandwidth -t h2d -i 100 | tee h2d.log
./ze_peak -t kernel_lat | tee lat.log


# bench from Fabrics team
cd $WORD_DIR
git clone -b ${L0_TESTS_BRANCH} --single-branch ${L0_TESTS_URL}
cd l0_tests/source
make all
./zet_benchmark -T 0 | tee d2h.log # Bandwidth
./zet_benchmark -T 3 | tee h2d.log # Kernel Launch Overhead
