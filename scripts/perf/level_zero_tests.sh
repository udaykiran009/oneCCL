#! /bin/bash

WORK_DIR="/home/gta/mshiryae"
LEVEL_ZERO_TESTS_URL="https://gitlab.devtools.intel.com/ict/ccl-team/level-zero-tests.git"
LEVEL_ZERO_TESTS_BRANCH="ccl"

sudo apt-get install libboost-all-dev
mkdir -p ${WORK_DIR}
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
