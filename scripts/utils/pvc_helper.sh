function check_cxi() {
    cxi_state=`fi_info -p cxi -v | grep -i state | grep -i up | wc -l`
    if [ "${cxi_state}" != 8 ]
    then
        check_command_exit_code 1 "\nCXI check failed\n"
    else
        echo_log "\nCXI check passed\n"
    fi
}

function check_anr() {
    test_path="/home/mshiryae/shared/zello_ipc_copy_dma_buf_p2p.out"
    total_fails=0

    for tile_idx in `seq 0 1`
    do
        for first_card_idx in `seq 0 5`
        do
            for second_card_idx in `seq $((first_card_idx+1)) 5`
            do
                check_log="anr_check_${first_card_idx}.${tile_idx},${second_card_idx}.${tile_idx}.log"
                cmd="ZE_AFFINITY_MASK=${first_card_idx}.${tile_idx},${second_card_idx}.${tile_idx} EnableCrossDeviceAccess=1"
                cmd="${cmd} ${test_path} > ${check_log} 2>&1"
                echo "${cmd}"
                timeout 10s bash -c "eval ${cmd}"

                test_exit_code=$?
                failed=$(cat ${check_log} | grep "FAILED" | wc -l)
                passed=$(cat ${check_log} | grep "PASSED" | wc -l)

                if [ "${failed}" == "1" ] || [ "${test_exit_code}" != "0" ]
                then
                    echo "config: ${first_card_idx}.${tile_idx}:${second_card_idx}.${tile_idx} failed"
                    total_fails=$((total_fails + 1))
                else
                    rm ${check_log}
                fi
            done
        done
    done

    if [ "${total_fails}" != "0" ]
    then
        check_command_exit_code 1 "\nANR check failed\n"
    else
        echo_log "\nANR check passed\n"
    fi
}

function set_pvc_mpich_ofi_env()
{
    module unload mpich/icc-cxi
    module load mpich/icc-cxi/43.2
    export LD_LIBRARY_PATH=/usr/lib64/:${LD_LIBRARY_PATH}
}

function set_agama_env()
{
    module use -a /home/ftartagl/graphics-compute-runtime/modulefiles
    module load graphics-compute-runtime/agama-ci-prerelease-260
}

function set_build_env() {
    module unload oneapi
    module use /home/ftartagl/modulefiles
    module load oneapi-testing/2021.4.0.001.rc1.20211005
}
