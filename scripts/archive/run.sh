#!/bin/sh

## Usage:
## sh ./run.sh <num nodes> "<binary name>" <num threads> <num endpoints per node> ep <application ppn; assume 1>
## Example: 16 nodes, 1 rank per node, 120 threads per rank, 2 endpoints per rank
## sh ./run.sh 8 "numactl -m 1 bin/cnn_avx512_perf_mn.bin config_file model_zoo/overfeat.solver 10240 5120" 120 2 ep 1

if [ "$MPIEP_HOME" != "" ]; then
    export LD_LIBRARY_PATH=$MPIEP_HOME:$LD_LIBRARY_PATH
else
    echo "export MPIEP_HOME=<path to EP-lib>"
    exit
fi

#cat $PBS_NODEFILE | sort > cpuhostfile

numnodes=$1
bin=$2
numthreads=$3
numendpoints=$4
mpiver=$5
ppncpu=$6
fabric=$7
ppnproxy=$((ppncpu*numendpoints))
totproxy=$((numnodes*numendpoints))

if [ "$fabric" != "openmpi" ]; then
    if [ "$fabric" == "" ] || [ "$fabric" == "ofi" ]; then
	if [ "$mpiver" == "ep" ]; then
	    # export PSM2_MQ_RNDV_HFI_WINDOW=4194304
	    # export PSM2_MQ_EAGER_SDMA_SZ=65536
	    # export PSM2_MQ_RNDV_HFI_THRESH=200000
	    echo "Check MPI environment variables"
	else
	    unset PSM2_MQ_RNDV_HFI_WINDOW
	    unset PSM2_MQ_EAGER_SDMA_SZ
	    unset PSM2_MQ_RNDV_HFI_THRESH
	fi
	export PSM2_IDENTIFY=1
	# export PSM2_RCVTHREAD=0
	export PSM2_SHAREDCONTEXTS=0

	if [ "$fabric" == "ofi" ]; then
	    export I_MPI_FABRICS=ofi
	else
	    export I_MPI_FABRICS=tmi
	    export I_MPI_TMI_PROVIDER=psm2
	fi
    elif [ "$fabric" == "dapl" ]; then
	export I_MPI_FABRICS=dapl
	export I_MPI_DAPL_PROVIDER=ofa-v2-hfi1_0-1
    else
	echo "Unknown I_MPI_FABRICS"
	exit
    fi

    export HFI_NO_CPUAFFINITY=1
    export IPATH_NO_CPUAFFINITY=1
    export I_MPI_FALLBACK=0
    export I_MPI_DYNAMIC_CONNECTION=0
    export I_MPI_SCALABLE_OPTIMIZATION=0
else
    . /opt/crtdc/openmpi/1.10.0-hfi-v3/bin/mpivars.sh
    . /opt/crtdc/openmpi/scripts/mpivars_help.sh
    PATH=/opt/crtdc/openmpi/1.10.0-hfi-v3/bin:${PATH}; export PATH
    export LD_LIBRARY_PATH=/opt/intel/compiler/2016u2/compilers_and_libraries_2016.2.181/linux/compiler/lib/intel64/:$LD_LIBRARY_PATH
    KNLOPAOPTSOPENMPI="-mca mtl psm2 -mca btl ^sm -x EPLIB_SHM_SIZE_GB=2 -x EPLIB_SERVER_AFFINITY=6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21 -x EPLIB_MAX_EP_PER_TASK=$numendpoints -x LD_LIBRARY_PATH=/opt/crtdc/openmpi/1.10.0-hfi-v3/lib64:/opt/intel/compiler/2016u2/compilers_and_libraries_2016.2.181/linux/compiler/lib/intel64/:$LD_LIBRARY_PATH"
    # KNLOPAOPTSOPENMPI+="-x PSM2_MQ_RNDV_HFI_WINDOW=4194304 -x PSM2_MQ_EAGER_SDMA_SZ=65536 -x PSM2_MQ_RNDV_HFI_THRESH=200000"
fi

# Server affinity
listep=255,254,253,252,251,250,249,248 # bin3
listep=271,270,269,268,267,266,265,264 # bin1
listep=7,8,9,10,11,12,13,14
listep=3,4,5,6,7,8,9,10
listep=6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21

# Client affinity
maxthreadid=$((numthreads-1))
maxcores=`cpuinfo | grep "Cores per package" | awk '{print $5}'`
affinitystr="proclist=[0-$maxthreadid],granularity=thread,explicit"
affinitystr="proclist=[0-5,15-67,68-73,83-135],granularity=thread,explicit"
affinitystr="proclist=[0-5,14-67,68-73,82-135],granularity=thread,explicit"
affinitystr="proclist=[0-2,$((2+numendpoints+1))-67,68-70,$((70+numendpoints+1))-135],granularity=thread,explicit"
affinitystr="proclist=[0-2,$((2+numendpoints+1))-$((maxcores-1)),$maxcores-$((maxcores+2)),$((maxcores+2+numendpoints+1))-$((2*maxcores-1))],granularity=thread,explicit"
affinitystr="proclist=[0-5,$((5+numendpoints+1))-$((maxcores-1)),$maxcores-$((maxcores+5)),$((maxcores+5+numendpoints+1))-$((2*maxcores-1))],granularity=thread,explicit"
#affinitystr="proclist=[0-5,$((5+numendpoints+1))-$((maxcores-1))],granularity=thread,explicit"
affinitystr="proclist=[3-5,$((5+numendpoints+1))-$((maxcores-1)),$((maxcores+3))-$((maxcores+5)),$((maxcores+5+numendpoints+1))-$((2*maxcores-1))],granularity=thread,explicit"
#affinitystr="proclist=[3-5,$((5+numendpoints+1))-$((maxcores-1))],granularity=thread,explicit"

## Platform independent script
export PCLDNN_SHM_SIZE_GB=2
export PCLDNN_NUM_SERVERS_T=$numendpoints
export PCLDNN_NUM_SERVERS_C=4
export PCLDNN_NUM_SERVERS_X=2
export PCLDNN_NUM_SERVERS_I=4
export PCLDNN_IO_THREAD_AFFINITY=0
export PCLDNN_SERVER_AFFINITY="$listep"

#ENV1="-genv I_MPI_DEBUG 5"
ENV2=""
#CPRO="-genv I_MPI_STATS ipm -genv I_MPI_STATS_FILE stats-$numnodes-$numthreads-$numendpoints-$ppncpu.txt"
#SPRO="-env I_MPI_STATS ipm -env I_MPI_STATS_FILE serverstats-$numendpoints-$numnodes.txt"
dir=`date +%M`
#mkdir -p vtune.$numnodes.$numthreads.$numendpoints.$dir/d
#CPRO="amplxe-cl -r vtune.$numnodes.$numthreads.$numendpoints.$dir/d -collect hotspots --"
#CPRO="amplxe-cl -r vtune.$numnodes.$numthreads.$numendpoints.$dir/d -collect concurrency --"
#SPRO="amplxe-cl -r vtune/server-$dir -collect hotspots --"

#CENV="$ENV1 $ENV2 -env I_MPI_PIN_DOMAIN [$mask]"
CENV="$ENV1 $ENV2"
CENVEP="$CENV"
CENVMT="$CENV"
SENV="$ENV1 $ENV2 -genv I_MPI_PIN_PROCESSOR_LIST \"$listep\""

if [ "$mpiver" == "ep" ]; then
    if [ "$fabric" != "openmpi" ]; then
	echo "mpiexec.hydra -ppn $ppnproxy -np $totproxy -hostfile cpuhostfile $SENV $SPRO numactl -m 0 $MPIEP_HOME/pcldnn_server$suffix"
	mpiexec.hydra -ppn $ppnproxy -np $totproxy -hostfile cpuhostfile $SENV $SPRO numactl -m 0 $MPIEP_HOME/pcldnn_server$suffix &
    else
	echo "mpirun -npernode $ppnproxy -np $totproxy -hostfile cpuhostfile $KNLOPAOPTSOPENMPI taskset -c "$listep" numactl -m 1 $MPIEP_HOME/pcldnn_server$suffix"
	mpirun -npernode $ppnproxy -np $totproxy -hostfile cpuhostfile $KNLOPAOPTSOPENMPI taskset -c $listep numactl -m 1 $MPIEP_HOME/pcldnn_server$suffix &
    fi
fi

if [ "$ppncpu" == "1" ]; then
    export I_MPI_PIN_MODE=lib
    export I_MPI_PIN_DOMAIN=node
    export KMP_AFFINITY=$affinitystr
    export OMP_NUM_THREADS=$numthreads
    if [ "$numthreads" -gt "$maxcores" ]; then
	export KMP_PLACE_THREADS=2t
    else
	export KMP_PLACE_THREADS=1t
    fi
    echo THREAD SETTINGS: Affinity $affinitystr Threads $numthreads Placement $KMP_PLACE_THREADS
else
    numcores=$((numthreads))
    export I_MPI_PIN_DOMAIN=[FFFF,FFFF0000,FFFF00000000,FFFF000000000000]
    export KMP_AFFINITY=compact,granularity=core
    export KMP_PLACE_THREADS=1t
    export OMP_NUM_THREADS=$numthreads
    export I_MPI_DEBUG=5
    echo "Ensure affinity for PPN>1 works."
fi

if [ "$fabric" != "openmpi" ]; then
    echo "mpiexec.hydra -ppn $ppncpu -np $numnodes -hostfile cpuhostfile $CENV $CPRO $bin"
    if [ "$mpiver" == "ep" ]; then
	sleep 30
    fi
    mpiexec.hydra -ppn $ppncpu -np $numnodes -hostfile cpuhostfile $CENV $CPRO $bin
else
    echo "mpirun -npernode $ppncpu -np $numnodes -hostfile cpuhostfile $KNLOPAOPTSOPENMPI $bin"
    if [ "$mpiver" == "ep" ]; then
	sleep 10
    fi
    mpirun -npernode $ppncpu -np $numnodes -hostfile cpuhostfile $KNLOPAOPTSOPENMPI -x KMP_AFFINITY="$affinitystr" -x OMP_NUM_THREADS=$numthreads -x KMP_PLACE_THREADS=1t $bin
fi

sleep 5

if [ "$mpiver" == "ep" ]; then
    rm -rf /dev/shm/*_shm_file-* /tmp/*_shm_file-*
    cpuhostlist=`cat cpuhostfile | awk '{printf("%s ", $1)}'`
    for host in $cpuhostlist; do
	ssh $host "rm -rf /dev/shm/pcldnn_shm_file-* /tmp/*_shm_file-* /dev/shm/psm2_*; killall -q mpiexec.hydra pcldnn_server$suffix" &
	#ssh $host "shmlist=`ipcs -a | awk '{print $1}' | grep 0x | awk '{printf(\"%s \", $1)}'`; for j in $shmlist; do ipcrm -M $j; done"
    done
fi
