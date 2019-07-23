Generic Workflow
=================

:guilabel:`Update instructions`.

Below is a generic flow of using the oneAPI CCL in C++:

#. Initialize the library:

   .. code-block::

      ccl::environment env;

#. Create a communicator objects:

   .. code-block::
  
     ccl::communicator comm; 

#. Execute collective operation of choice on this communicator:

   .. code-block::

      auto request = comm.allreduce(...);
      request->wait();
