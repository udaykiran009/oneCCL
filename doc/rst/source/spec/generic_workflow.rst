Generic Workflow
=================

Below is a generic flow for using C++ API of oneCCL:

1. Initialize the library:

::

        ccl::environment::instance();

2. or just Create communicator objects:

::

        ccl::communicator_t comm = ccl::environment::instance().create_communicator();

3. Execute collective operation of choice on this communicator:

::

        auto request = comm.allreduce(...);
        request->wait();
