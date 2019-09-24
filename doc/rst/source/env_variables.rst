Environment variables
=====================

:guilabel:`Add description for algorithms`.

CCL_ATL_TRANSPORT
#################
**Syntax**

``CCL_ATL_TRANSPORT=<value>``

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``mpi``
     - MPI transport (**default**).
   * - ``ofi``
     - OFI (libfaric) transport.

**Description**

Set this environment variable to select transport for inter-node communications.

Collective algorithms selection
###############################

CCL_<coll_name>
***************
**Syntax**

- ``CCL_<coll_name>=<algo_name>`` sets the specific algorithm for the whole message size range.
- ``CCL_<coll_name>="<algo_name_1>[:<size_range_1>][;<algo_name_2><size_range_2>][;...]"`` 
  the list of semicolon separated blocks where each block sets the specific algorithm for specific message size range.


``<coll_name>`` is selected from a list of available collective operations and ``<algo_name>`` is selected from a list of available algorithms for a specific collective operation (listed below).

``<size_range>`` is described by left and right size borders in format ``<left>-<right>``. Size is specified in bytes. Reserved word ``max`` can be used to specify the maximum message size.

oneCCL internally fills algorithm selection table with sensible defaults. User input complements the selection table. To see the actual table values set CCL_LOG_LEVEL=1.

Example
*******

``CCL_ALLREDUCE="recursive_doubling:0-8192;rabenseifner:8193-1048576;ring:1048577-max"``

Available collectives
*********************

``<coll_name>``

-   ``ALLGATHER``
-   ``ALLREDUCE``
-   ``BARRIER``
-   ``BCAST``
-   ``REDUCE``
-   ``SPARSE_ALLREDUCE``


Available algorithms
********************

``<algo_name>``

``ALLGATHERV`` algorithms
+++++++++++++++++++++++++

.. list-table:: 
   :widths: 25 50
   :align: left
   
   * - ``direct``
     - Based on MPI_Iallgatherv
   * - ``naive``
     - Send to all, receive from all
   * - ``flat``
     - Alltoall-based allgorithm
   * - ``multi_bcast``
     - Series of broadcast operations with different root ranks


``ALLREDUCE`` algorithms
++++++++++++++++++++++++

.. list-table:: 
   :widths: 25 50
   :align: left

   * - ``direct``
     - Based on MPI_Iallreduce
   * - ``rabenseifner``
     - Rabenseifner’s algorithm
   * - ``starlike``
     - May be beneficial for imbalanced workloads
   * - ``ring`` 
     - reduce_scatter+allgather ring
   * - ``ring_rma``
     - reduce_scatter+allgather ring using RMA communications
   * - ``double_tree``
     - Double-tree algorithm
   * - ``recursive_doubling``
     - Recursive doubling algorithm


``BARRIER`` algorithms
++++++++++++++++++++++

.. list-table:: 
   :widths: 25 50
   :align: left
   
   * - ``direct``
     - Based on MPI_Ibarrier
   * - ``ring``
     - Ring-based allgorithm


``BCAST`` algorithms
++++++++++++++++++++

.. list-table:: 
   :widths: 25 50
   :align: left

   * - ``direct``
     - Based on MPI_Ibcast
   * - ``ring`` 
     - Ring.
   * - ``double_tree``
     - Double-tree algorithm
   * - ``naive``
     - Send to all from root rank


``REDUCE`` algorithms
+++++++++++++++++++++

.. list-table:: 
   :widths: 25 50
   :align: left

   * - ``direct``
     - Based on MPI_Ireduce
   * - ``rabenseifner``
     - Rabenseifner’s algorithm
   * - ``tree``
     - Tree algorithm
   * - ``double_tree``
     - Double-tree algorithm


``SPARSE_ALLREDUCE`` algorithms
+++++++++++++++++++++++++++++++

.. list-table:: 
   :widths: 25 50
   :align: left

   * - ``basic``
     - Basic allgorithm
   * - ``mask``
     - Mask-based allgorithm


CCL_FUSION
##########
**Syntax**

``CCL_FUSION=<value>``

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``1``
     - Enable fusion of collective operations
   * - ``0``
     - Disable fusion of collective operations (**default**)

**Description**

Set this environment variable to control fusion of collective operations. The real fusion will depend on additional settings described below.


CCL_FUSION_BYTES_THRESHOLD
##########################
**Syntax**

``CCL_FUSION_BYTES_THRESHOLD=<value>``

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``SIZE``
     - Bytes threshold for collective operation. If the size of communication buffer in bytes is less or equal
       to ``SIZE`` then oneCCL will fuse this operation with other ones.

**Description**

Set this environment variable to specify bytes threshold for collective operation to be fused.


CCL_FUSION_COUNT_THRESHOLD
##########################
**Syntax**

``CCL_FUSION_COUNT_THRESHOLD=<value>``

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``COUNT``
     - Count threshold for collective operations.
       oneCCL can fuse together no more than ``COUNT`` operations at a time.

**Description**

Set this environment variable to specify count threshold for collective operation to be fused.


CCL_FUSION_CYCLE_MS
###################
**Syntax**

``CCL_FUSION_CYCLE_MS=<value>``

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``MS``
     - Frequency of checking for collectives operations to be fused, in milliseconds.
       Small ``MS`` value can improve latency. Large value can help
       to fuse larger number of operations at a time.

**Description**

Set this environment variable to specify frequency of checking for collectives operations to be fused.


CCL_UNORDERED_COLL
##################
**Syntax**

``CCL_UNORDERED_COLL=<value>``

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``1``
     - Enable execution of unordered collectives.
       Additionally user has to specify ``coll_attr.match_id``.
   * - ``0``
     - Disable execution of unordered collectives (**default**).

**Description**

Set this environment variable to enable execution of unordered collective operations on different nodes. 


CCL_PRIORITY
############
**Syntax**

``CCL_PRIORITY=<value>``

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``direct``
     - Priority is explicitly specified by users using coll_attr.priority.
   * - ``lifo``
     - Priority is implicitly increased on each collective calls. Users do not specify a priority.
   * - ``none``
     - Disable prioritization (**default**).

**Description**

Set this environment variable to control priority mode of collective operations. 


CCL_WORKER_COUNT
################
**Syntax**

``CCL_WORKER_COUNT=<value>``

**Arguments**

.. list-table:: 
   :widths: 25 50
   :header-rows: 1
   :align: left
   
   * - <value> 
     - Description
   * - ``N``
     - Number of worker threads for oneCCL rank. 1 if not specified.

**Description**

Set this environment variable to specify number of CCL worker threads.


CCL_WORKER_AFFINITY
###################
**Syntax**

``CCL_WORKER_AFFINITY=<proclist>``

**Arguments**

.. list-table:: 
   :widths: 25 50
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



CCL_PM_TYPE
###########
**Syntax**

``CCL_PM_TYPE=<value>``

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``0``
     - Use PMI (process manager interface) with mpirun (**default**).
   * - ``1``
     - Use internal KVS (key-value storage) without mpirun.

**Description**

Set this environment variable to specify process manager type.


CCL_KVS_IP_EXCHANGE
###################
**Syntax**

``CCL_KVS_IP_EXCHANGE=<value>``

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``0``
     - Use K8S for ip exchange (**default**).
   * - ``1``
     - Use specific environment to get master ip.

**Description**

Set this environment variable to specify the way to IP addresses of ran processes are exchanged.


CCL_K8S_API_ADDR
################
**Syntax**

``CCL_K8S_API_ADDR =<value>``

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``IP:PORT``
     - Set address and port of k8s kvs.

**Description**

Set this environment variable to specify k8s kvs address.


CCL_K8S_MANAGER_TYPE
####################
**Syntax**

``CCL_K8S_MANAGER_TYPE=<value>``

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``none``
     - Use Pods labels for ip exchange (**default**).
   * - ``k8s``
     - Use Statefulset\Deployment labels for ip exchange.

**Description**

Set this environment variable to specify way to ip exchange.


CCL_KVS_IP_PORT
###############
**Syntax**

``CCL_KVS_IP_PORT=<value>``

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``IP:PORT``
     - Set address and port of master kvs server.

**Description**

Set this environment variable to specify master kvs address.


CCL_WORLD_SIZE
##############
**Syntax**

``CCL_WORLD_SIZE=<value>``

**Arguments**

.. list-table::
   :widths: 25 50
   :header-rows: 1
   :align: left

   * - <value>
     - Description
   * - ``N``
     - Number of processes to start execution.

**Description**

Set this environment variable to specify number of oneCCL processes.

