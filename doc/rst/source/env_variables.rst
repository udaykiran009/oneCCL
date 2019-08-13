Environment variables
==========================

:guilabel:`To be added`.

CCL_ATL_TRANSPORT
####################

CCL_ALLGATHERV
####################
**Syntax**

``CCL_ALLGATHERV=<value>``

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

Set this environment variable to specify algorithm choice for Allgatherv.

CCL_ALLREDUCE
###################
**Syntax**

``CCL_ALLREDUCE=<value>``

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

CCL_BARRIER
###########
**Syntax**

``CCL_BARRIER=<value>``

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``fake item``
     - fake item description

**Description**

Set this environment variable to specify algorithm choice for Barrier.

CCL_BCAST
#########
**Syntax**

``CCL_BCAST=<value>``

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``fake item``
     - fake item description

**Description**

Set this environment variable to specify algorithm choice for Bcast.

CCL_REDUCE
##########
**Syntax**

``CCL_REDUCE=<value>``

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``fake item``
     - fake item description

**Description**

Set this environment variable to specify algorithm choice for Reduce.

CCL_SPARSE_ALLREDUCE
####################
**Syntax**

``CCL_SPARSE_ALLREDUCE=<value>``

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``fake item``
     - fake item description

**Description**

Set this environment variable to specify algorithm choice for sparse Allreduce.

CCL_FUSION
##########

CCL_FUSION_BYTES_THRESHOLD
##########################

CCL_FUSION_COUNT_THRESHOLD
##########################

CCL_FUSION_CYCLE_MS
###################

CCL_UNORDERED_COLL
##################
**Syntax**

``CCL_UNORDERED_COLL=<value>``

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

CCL_PRIORITY
############
**Syntax**

``CCL_PRIORITY=<value>``

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

CCL_RANK_COUNT
##############

CCL_ASK_FRAMEWORK
#################

CCL_KUBE_API_ADDR
#################

CCL_WORKER_AFFINITY
###################
**Syntax**

``CCL_WORKER_AFFINITY=<proclist>``

**Arguments**

.. list-table:: 
   :header-rows: 1
   :align: left
   
   * - <proclist> 
     - Description
   * - ``n1,n2,..``
     - Affinity is explicitly specified by user.
   * - ``auto``
     - Workers are pinned to K last cores of pin domain where K is CCL_WORKER_COUNT (**default**). 

**Description**

Set this environment variable to specify cpu affinity for CCL worker threads.


CCL_WORKER_COUNT
################
**Syntax**

``CCL_WORKER_COUNT=<value>``

**Arguments**

.. list-table:: 
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``N``
     - Number of worker threads for CCL rank. 2 if not specified.

**Description**

Set this environment variable to specify number of CCL worker threads.


