.. highlight:: bash

Deep Learning Optimizations
===========================

Set Workers Affinity 
*********************

There are two ways to set workers threads affinity: explicit and automatic.

Explicit setup
##############

To set affinity explicitly, follow these steps:

#. Pass the number of workers threads needed to the ``CCL_WORKER_COUNT`` environment variable.

#. Pass ID of the cores to be bound to to  the ``CCL_WORKER_AFFINITY`` environment variable. 

Example
+++++++

In the example below, CCL creates 4 threads and pins them to cores with numbers 3, 4, 5, and 6 accordingly:
::

   export CCL_WORKER_COUNT=4
   export CCL_WORKER_AFFINITY=3,4,5,6

Automatic setup
###############

To set affinity automatically, follow these steps:

#. Pass the number of workers threads needed to the ``CCL_WORKER_COUNT`` environment variable.

#. Set ``CCL_WORKER_AFFINITY`` to ``auto``. 

.. note:: Automatic pinning only works if application is launched using ``mpirun`` provided by the CCL distribution package.

Persistent collective operations
********************************

Collective operations may have expensive initialization phase (like allocation of internal structures/buffers, registration of memory buffers, handshake with peers, etc.).
CCL amortizes these overheads by caching collective internal representation and reusing them on subsequent calls.

To control, use ``coll_attr.to_cache = 1``, which is enabled by default.


Prioritization for collective operations
****************************************

CCL allows for specification of priority for collective operation. Priority – non-negative number, higher number – higher priority (i.e. more urgent execution).

oneAPI CCL supports 2 prioritization modes:

-	Direct - Priority is explicitly specified by users using coll_attr.priority.
-	LIFO - Priority is implicitly increased on each collective calls. User do not specify a priority.

To control the mode, pass ``none``, ``direct``, ``lifo`` to the CCL_PRIORITY_MODE environment variable. By default, prioritization is disabled (``none``).

Manual selection of collective algorithms
*****************************************

You can manually select collective algorithm using ``CCL_ALLREDUCE_ALGO``.

-	ring – reduce_scatter+allgather ring
-	ring_rma - reduce_scatter+allgather ring using rma communications
-	Starlike – may be beneficial for imbalanced workloads
-	tree – Rabenseifner’s algorithm (default)


The default algorithm for small messages is recursive-doubling, for large messages - Rabenseifner’s.

Out of order execution
**********************

Collective operations may be executed out of order on different nodes due to network or hardware-specific reasons.

In some implementations, such as out of order execution, a hang or data corruption may occur.
oneAPI CCL provides a mechanism to arrange collective operations execution in accordance with the user-defined identifier.

To control this, use the environment variable ``CCL_OUT_OF_ORDER_SUPPORT``

	The user can set an identifier using ccl_coll_attr_t. match_id  field which is a pointer to a null terminated C-style string
	Out of order execution is controlled by the rank with id zero, i.e. by the root rank.
	When the root rank receives a request from the user with non empty match_id for the first time, it broadcasts information about the user identifier to all other ranks and assigns an internal CCL identifier which will be used with all the following operations with the same match_id.
	When non root rank receives a request from the user with non empty match_id for the first time, it postpones operation execution until it receives a message from the root rank. Once the message is received, rank creates an internal CCL identifier which will be used for all the following operations with the same match_id.

