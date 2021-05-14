function run_in_fake_container() {
    # select appropriate fakechroot version

    if [[ $(lsb_release -d) == *"Ubuntu"* ]]; then
        ub_majver=$(lsb_release -sr | grep -Eo '^[0-9]+')
        fakechroot_clone=/p/pdsd/opt/lin/fake-utils/u$ub_majver/clone.sh
    elif [[ $(lsb_release -d) == *"Red Hat"* ]]; then 
        el_majver=$(rpm --eval "%{rhel}")
        fakechroot_clone=/p/pdsd/opt/lin/fake-utils/el$el_majver/clone.sh
    fi
    
    # create light clone of current system in $PWD/fake_container
    "$fakechroot_clone" --create fake_container

    # prepare build workspace in $PWD/fake_container/build/impi
    mkdir -p fake_container/build/ccl
    find -mindepth 1 -maxdepth 1 -not -name fake_container -exec mv -t fake_container/build/ccl '{}' '+'

    # do the job
    "$fakechroot_clone" --run fake_container "$@" && err=0 || err=$?

    # move everything back
    find fake_container/build/ccl -mindepth 1 -maxdepth 1 -exec mv -t . '{}' '+'

    rm -rf fake_container

    return $err
}
