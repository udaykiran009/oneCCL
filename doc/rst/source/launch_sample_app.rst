Launch Sample Application
=========================

:guilabel:`Update sample`.

Intel® oneAPI CCL supplies a sample application, ``iccl_test.c`` or ``iccl_test.cpp`` which demonstrates
the usage of ICCL API.

Launching the Sample
--------------------

1. For the C++ sample, build ``iccl_test.cpp``:

.. code-block::
   $ cd <install_dir>/test
   $ icpc -O2 –I<install_dir>/intel64/include/ -L<install_dir>/intel64/lib
   -liccl -ldl -lrt -lpthread -o iccl_test iccl_test.cpp
   
2. Launch the iccl_test binary with mpirun on the desired number of nodes (N).

Sample Description
------------------
The application is set up to run a test for two layers. It sets output on the 1st layer and checks input for
the 2nd layer in an fprop() call. Similarly, for the bprop() call, it sets a gradient with respect to input
for the 2nd layer and checks the gradient with respect to output for the 1
st layer. For weights, it sets gradients with respect to weights, checks the gradients accumulation, modifies weights in a wtinc()
call, and then verifies the expected values in an fprop() call for both layers.
The application prints parameters for input and output feature maps and weights, whether the
communication is required, and what type of communication is required. The test status is printed as
PASSED or FAILED. You can grep for FAILED to see if a test failed.
