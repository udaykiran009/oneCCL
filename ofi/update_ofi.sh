#!/bin/bash

ofi_rt_path=`pwd`

git clone https://github.com/ofiwg/libfabric.git
cd ./libfabric
./autogen.sh
ofi_install_path=`pwd`/_install
./configure --prefix=$ofi_install_path \
    --enable-atomics=no \
    --enable-psm2=dl \
    --enable-psm3=dl \
    --enable-rxm=dl \
    --enable-shm=dl \
    --enable-sockets=dl \
    --enable-tcp=dl \
    --enable-verbs=dl \
    --disable-bgq \
    --disable-efa \
    --disable-gni \
    --disable-hook_debug \
    --disable-mlx \
    --disable-mrail \
    --disable-perf \
    --disable-psm \
    --disable-rxd \
    --disable-rstream \
    --disable-static \
    --disable-udp \
    --disable-usnic \
    --disable-xpmem \
    --with-build_id='-ccl'

make -j all && make install

cp -f $ofi_install_path/bin/fi_info $ofi_rt_path/bin
cp -rf $ofi_install_path/include/rdma $ofi_rt_path/include
cp -f -T $ofi_install_path/lib/libfabric.so.1.* $ofi_rt_path/lib/libfabric.so.1
cp -f $ofi_install_path/lib/libfabric/libpsmx2-fi.so $ofi_rt_path/lib/prov
cp -f $ofi_install_path/lib/libfabric/libpsm3-fi.so $ofi_rt_path/lib/prov
cp -f $ofi_install_path/lib/libfabric/librxm-fi.so $ofi_rt_path/lib/prov
cp -f $ofi_install_path/lib/libfabric/libshm-fi.so $ofi_rt_path/lib/prov
cp -f $ofi_install_path/lib/libfabric/libsockets-fi.so $ofi_rt_path/lib/prov
cp -f $ofi_install_path/lib/libfabric/libtcp-fi.so $ofi_rt_path/lib/prov
cp -f $ofi_install_path/lib/libfabric/libverbs-fi.so $ofi_rt_path/lib/prov
