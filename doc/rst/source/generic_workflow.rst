Generic Workflow
=================

:guilabel:`Update instructions`.

Below is a generic flow of using the Intel® CCL in C++:

#. Initialize the library:

   .. code-block::

      iccl::environment env;

#. Create a communicator objects:

   .. code-block::
  
     iccl::communicator comm; 

#. Execute collective operation of choice on this communicator:

   .. code-block::

      auto request = comm.allreduce(...);
      request->wait();
