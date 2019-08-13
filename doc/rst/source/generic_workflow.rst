Generic workflow
=================

:guilabel:`To be added`.

Below is a generic flow of using the oneAPI CCL in C++:

1. Initialize the library:

::

        ccl::environment env;

2. Create a communicator objects:

::  

        ccl::communicator comm; 

3. Execute collective operation of choice on this communicator:

::

        auto request = comm.allreduce(...);
        request->wait();
