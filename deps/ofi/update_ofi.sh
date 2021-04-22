#!/bin/bash

runtime_path=`pwd`

git clone https://github.com/ofiwg/libfabric.git
cd ./libfabric
./autogen.sh
install_path=`pwd`/_install
./configure --prefix=$install_path \
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

cp -f $install_path/bin/fi_info $runtime_path/bin
cp -rf $install_path/include/rdma $runtime_path/include
cp -f -T $install_path/lib/libfabric.so.1.* $runtime_path/lib/libfabric.so.1
cp -f $install_path/lib/libfabric/libpsmx2-fi.so $runtime_path/lib/prov
cp -f $install_path/lib/libfabric/libpsm3-fi.so $runtime_path/lib/prov
cp -f $install_path/lib/libfabric/librxm-fi.so $runtime_path/lib/prov
cp -f $install_path/lib/libfabric/libshm-fi.so $runtime_path/lib/prov
cp -f $install_path/lib/libfabric/libsockets-fi.so $runtime_path/lib/prov
cp -f $install_path/lib/libfabric/libtcp-fi.so $runtime_path/lib/prov
cp -f $install_path/lib/libfabric/libverbs-fi.so $runtime_path/lib/prov
