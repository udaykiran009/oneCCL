-----------------------------------
Short README for ICCL build and test
-----------------------------------

1. Requirements
   ICC and MPI compiler sourced

$source  /opt/intel/compilers_and_libraries/linux//bin/compilervars.sh intel64

2. Build:
   Simply type "make" in root directory. By default DEBUG mode is off. You can turn it on using "make DEBUG=1".
   This builds libiccl.a library and iccl_test test application in bin directory.

3. Run test app
   Simply run iccl_test using mpiexec.hydra with desired number of nodes (N). It takes single parameter "num_groups"
      num_groups = 1 -- Data parallelism
      num_groups = N -- Model parallelism
      num_groups > 1 and num_groups < N -- Hybrid parallelism
   Example run commands for NUM_NODES nodes:

   NUM_NODES=8
   # Data parallelism
   $mpiexec.hydra -np ${NUM_NODES} bin/iccl_test 1

   # Model parallelism
   $mpiexec.hydra -np ${NUM_NODES} bin/iccl_test ${NUM_NODES}

   # Hybrid parallelism (NUM_NODES/2) x 2
   $mpiexec.hydra -np ${NUM_NODES} bin/iccl_test 2

4. Test setup and Sample Output
   Test app is setup to run test for 2 layers. It sets output on 1st layer and checks input for 2nd layer in fprop call.
   Similarly, for bprop, it sets delinput for 2nd layer and checks deloutput for 1st layer. For weights, it sets delwt and checks
   accumulation is wtinc and modifies weights in wtinc and checks expected values in fprop call for both the layers.
   App prints parameters for input and output feature maps and weights, whether communication is required and what type of
   communication is required. Then it runs the test and prints if test is PASSED or FAILED. Grep for FAIL to see if any test failed.

   mpiexec.hydra  -np 8 ./bin/iccl_test 2 > tests/sample_output.txt
