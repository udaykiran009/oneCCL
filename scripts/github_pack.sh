#!/bin/bash -x

dstdir="_package"
rm -rf $dstdir
rm -rf $dstdir.tgz

excluded=".git
.gitignore
.github
./oneccl_license.txt
./boms
./scripts
./doc/README.txt
./doc/cclEULA.txt
./doc/copyright
./examples/run.sh
./deps/hwloc/update_hwloc.sh
./deps/ofi/update_ofi.sh
./deps/itt/update_ittnotify.sh
./deps/itt/update_itt_calls.sh
./deps/itt/gen
./ccl_public
./ccl_oneapi
./tests/cfgs
./tests/reg_tests
./tests/reproducer
.tools"

count=0
mkdir -p $dstdir
cd ./../mlsl2

#preparing files
./scripts/preparing_files.sh ccl_public

ls -al

for srcfile in `find .`
do
    for exfile in $excluded
    do
        if [ `echo "${srcfile}" | grep  "${exfile}"` ]
        then
            count=$((++count))
            echo $count
        fi
    done
    if [ $count -eq 0  ]
    then
        dstfile=$srcfile
        echo $dstfile
        echo "new" $dstfile
        cp -PL --parents $srcfile $dstdir
    fi
    count=0
done

cd $dstdir

for CUR_FILE in `find .  -type f | grep -e "\.hpp$" -e "\.cpp$" -e "\.h$" -e "\.c$" | grep -v "googletest" | grep -v "deps"`
do
	ed $CUR_FILE < ../scripts/copyright/copyright.c ; \
done

for CUR_FILE in `find .  -type f | grep  -e "\.sh$" -e "\.sh.in$" | grep -v "googletest"`
do
	ed $CUR_FILE < ../scripts/copyright/copyright.sh > /dev/null 2>&1; \
done

for CUR_FILE in `find .  -type f | grep -e "\.py$"| grep -v "doc" | grep -v "googletest"`
do
	ed $CUR_FILE < ../scripts/copyright/copyright.m > /dev/null 2>&1; \
done

for CUR_FILE in `find .  -type f | grep -e "Makefile$" -e "CMakeLists.txt"| grep -v "doc" | grep -v "googletest"`
do
	ed $CUR_FILE < ../scripts/copyright/copyright.m > /dev/null 2>&1; \
done

for CUR_FILE in `find .  -type f | grep -e "\.cmake.in$" | grep -v "googletest"`
do
	ed $CUR_FILE < ../scripts/copyright/copyright.m > /dev/null 2>&1; \
done

# tar -czf ../$dstdir.tgz .
