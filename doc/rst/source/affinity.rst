.. highlight:: bash

Pinning
************************
By setting workers affinity, you can :guilabel:`info needed`.

There are two ways to set workers threads affinity: explicit and automatic.

Explicit setup
##############

To set affinity explicitly, follow these steps:

#. Pass the number of workers threads needed to the ``CCL_WORKER_COUNT`` environment variable.

#. Pass ID of the cores to be bound to to  the ``CCL_WORKER_AFFINITY`` environment variable. 

Example
+++++++

In the example below, CCL creates 4 threads and pins them to cores with numbers 3, 4, 5, and 6, respectively:
::

   export CCL_WORKER_COUNT=4
   export CCL_WORKER_AFFINITY=3,4,5,6

Automatic setup
###############

.. note:: Automatic pinning only works if application is launched using ``mpirun`` provided by the CCL distribution package.

To set affinity automatically, follow these steps:

#. Pass the number of workers threads needed to the ``CCL_WORKER_COUNT`` environment variable.

#. Set ``CCL_WORKER_AFFINITY`` to ``auto``. 

Example
+++++++

In the example below, CCL creates 4 threads and pins them to the last 4 cores available for the process launched:
::

   export CCL_WORKER_COUNT=4
   export CCL_WORKER_AFFINITY=auto

.. note:: The exact IDs of CPU cores depend on the parameters passed to ``mpirun``.
