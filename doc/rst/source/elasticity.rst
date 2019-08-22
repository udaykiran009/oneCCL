.. highlight:: bash

Fault tolerance / elasticity
############################

Main instructions
+++++++++++++++++

Before ranks launch user can specify :ref:`CCL_WORLD_SIZE` = N, where N - number of ranks to start.
If k8s with k8s manager support is used then N = replicasize by default.

User can specify own function which will decide what CCL should do on "world" resize event - wait / use current "world" information / finalize.
::

  typedef enum ccl_resize_action
  {
    // wait additional changes for number of ranks
    ccl_ra_wait     = 0,

    // run with current number of ranks
    ccl_ra_use      = 1,

    // finalize work
    ccl_ra_finalize = 2,

  } ccl_resize_action_t;

  typedef ccl_resize_action_t(*ccl_on_resize_fn_t)(size_t comm_size);

  ccl_set_resize_callback(ccl_on_resize_fn_t callback);

This function will be called on CCL level in case of changes in number of ranks. Application level (e.g. framework) should return action which CCL should perform.

Setting this function to NULL (default value) means that CCL will work exactly with :ref:`CCL_WORLD_SIZE` or replicasize ranks without fault tolerant / elasticity.


Examples
++++++++

To run ranks in k8s without k8s manager, like set of pods
*********************************************************

-   :ref:`CCL_PM_TYPE` = 1
-   :ref:`CCL_K8S_API_ADDR` = k8s server address and port, like IP:PORT
-   run your example


To run ranks in k8s use statefulset / deployment as manager
***********************************************************

-   :ref:`CCL_PM_TYPE` = 1
-   :ref:`CCL_K8S_API_ADDR` = k8s server address
-   :ref:`CCL_K8S_MANAGER_TYPE` = k8s
-   run your example


To run ranks without mpirun
***************************

-   :ref:`CCL_PM_TYPE` = 1
-   :ref:`CCL_KVS_IP_EXCHANGE` = 1
-   :ref:`CCL_KVS_IP_PORT` = ip_port of one of your node where you run example
-   run your example
