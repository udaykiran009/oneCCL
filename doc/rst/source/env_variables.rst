Environment variables
=====================

:guilabel:`Add description for algorithms`.

CCL_ATL_TRANSPORT
#################
**Syntax**

``CCL_ATL_TRANSPORT=<value>``

**Arguments**

.. list-table:: 
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``mpi``
     - MPI transport (**default**).
   * - ``ofi``
     - OFI (libfaric) transport.

**Description**

Set this environment variable to be select transport for inter-node communications.

Collective algorithms selection
###############################

CCL_<coll_name>
###############
**Syntax**

``CCL_<coll_name>=<algo_name>``

Sets the specific algorithm for the whole message size range.

or

``CCL_<coll_name>="<algo_name_1>[:<size_range_1>][;<algo_name_2><size_range_2>][;...]"``

The list of semicolon separated blocks where each block sets the specific algorithm for specific message size range.


``<coll_name>`` is selected from list of available collective operations and ``<algo_name>`` is selected from list of available algorithms for specific collective operation (listed below).

``<size_range>`` is described by left and right size borders in format ``<left>-<right>``. Size is specified in bytes. Reserved word ``max`` can be used to specify the maximum message size.

CCL internally fills algorithm selection table with sensible defaults. User input complements the selection table. To see the actual table values set CCL_LOG_LEVEL=1.

Example
#######

``CCL_ALLREDUCE="recursive_doubling:0-8192;rabenseifner:8193-‭1048576;ring:‬‭1048577-max"``

List of available ``<coll_name>``

-   ``ALLGATHER``
-   ``ALLREDUCE``
-   ``BARRIER``
-   ``BCAST``
-   ``REDUCE``
-   ``SPARSE_ALLREDUCE``


List of available ``ALLGATHERV`` algorithms
###########################################

.. list-table:: 
   :widths: 25 50
   :align: left
   
   * - ``direct``
     - Based on MPI_Iallgatherv.
   * - ``naive``
     - Send to all, receive from all.
   * - ``flat``
     - Alltoall-based allgorithm.
   * - ``multi_bcast``
     - Series of broadcast operations with different root ranks.


List of available ``ALLREDUCE`` algorithms
##########################################

.. list-table:: 
   :widths: 25 50
   :align: left

   * - ``direct``
     - Based on MPI_Iallreduce.
   * - ``rabenseifner``
     - Rabenseifner’s algorithm
   * - ``starlike``
     - May be beneficial for imbalanced workloads.
   * - ``ring`` 
     - reduce_scatter+allgather ring.
   * - ``ring_rma``
     - reduce_scatter+allgather ring using RMA communications.
   * - ``double_tree``
     - Double-tree algorithm.
   * - ``recursive_doubling``
     - Recursive doubling algorithm.


List of available ``BARRIER`` algorithms
########################################

.. list-table:: 
   :widths: 25 50
   :align: left
   
   * - ``direct``
     - Based on MPI_Ibarrier.
   * - ``ring``
     - Ring-based allgorithm.


List of available ``BCAST`` algorithms
######################################

.. list-table:: 
   :widths: 25 50
   :align: left

   * - ``direct``
     - Based on MPI_Ibcast.
   * - ``ring`` 
     - Ring.
   * - ``double_tree``
     - Double-tree algorithm.
   * - ``naive``
     - Send to all from root rank.


List of available ``REDUCE`` algorithms
#######################################

.. list-table:: 
   :widths: 25 50
   :align: left

   * - ``direct``
     - Based on MPI_Ireduce.
   * - ``rabenseifner``
     - Rabenseifner’s algorithm.
   * - ``tree``
     - Tree algorithm.
   * - ``double_tree``
     - Double-tree algorithm.


List of available ``SPARSE_ALLREDUCE`` algorithms
#################################################

.. list-table:: 
   :widths: 25 50
   :align: left

   * - ``basic``
     - Basic allgorithm.
   * - ``mask``
     - Mask-based allgorithm.


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

CCL_RANK_COUNT
##############

CCL_ASK_FRAMEWORK
#################

CCL_KUBE_API_ADDR
#################
