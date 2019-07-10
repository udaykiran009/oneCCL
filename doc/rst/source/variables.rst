Environment Variables
==========================

:guilabel:`Add information about variables`.

ICCL_ALLREDUCE_ALGO
###################
**Syntax**

``ICCL_ALLREDUCE_ALGO=<value>``

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``ring`` 
     - reduce_scatter+allgather ring.
   * - ``ring_rma``
     - reduce_scatter+allgather ring using rma communications.
   * - ``starlike``
     - may be beneficial for imbalanced workloads.
   * - ``tree``
     - Rabenseifner’s algorithm (**default**).

**Description**

Set this environment variable to specify algorithm choice for AllReduce. The default algorithm for small messages is recursive-doubling, for large messages - Rabenseifner’s.

ICCL_ALLGATHERV_ALGO
####################
**Syntax**

``ICCL_ALLGATHERV_ALGO=<value>``

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``bcast`` 
     - Series of broadcast operations with different roots (**default**).
   * - ``flat``
     - Alltoall-based approach.
   * - ``direct``
     - Based on MPI_Allgatherv.

**Description**

Set this environment variable to specify algorithm choice for AllReduce. The default algorithm for small messages is recursive-doubling, for large messages - Rabenseifner’s.

ICCL_ENABLE_FUSION
#########################

ICCL_FUSION_BYTES_THRESHOLD
###########################

ICCL_FUSION_COUNT_THRESHOLD
###########################

ICCL_FUSION_CYCLE_MS
####################

ICCL_OUT_OF_ORDER_SUPPORT
#########################
**Syntax**

``ICCL_OUT_OF_ORDER_SUPPORT=<value>``

**Arguments**

.. list-table:: 
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``1``
     - Enable out of order execution.
   * - ``0``
     - Disable out of order execution (**default**).

**Description**

Set this environment variable to enable out of order execution of collective operations on different nodes. 

ICCL_PRIORITY_MODE
###################
**Syntax**

``ICCL_PRIORITY_MODE=<value>``

**Arguments**

.. list-table:: 
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``direct``
     - Priority is explicitly specified by users using coll_attr.priority.
   * - ``lifo``
     - Priority is implicitly increased on each collective calls. 

       Users do not specify a priority.
   * - ``none``
     - Disable prioritization (**default**).

**Description**

Set this environment variable to be able to control priority for collective operations. 

ICCL_RANK_COUNT
####################

ICCL_ASK_FRAMEWORK
####################

ICCL_KUBE_API_ADDR
####################

ICCL_WORKER_AFFINITY
####################
**Syntax**

``ICCL_WORKER_AFFINITY=<proclist>``

**Arguments**

.. list-table:: 
   :header-rows: 1
   :align: left
   
   * - <proclist> 
     - Description
   * - ``n1,n2,..``
     - Affinity is explicitly specified by user.
   * - ``auto``
     - Workers are pinned to K last cores of pin domain where K is ICCL_WORKER_COUNT (**default**). 

**Description**

Set this environment variable to specify cpu affinity for ICCL worker threads.


ICCL_WORKER_COUNT
###################
**Syntax**

``ICCL_WORKER_COUNT=<value>``

**Arguments**

.. list-table:: 
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``N``
     - Number of worker threads for MLSL rank. 2 if not specified.

**Description**

Set this environment variable to specify number of ICCL worker threads.


