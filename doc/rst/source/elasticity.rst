.. highlight:: bash

Fault tolerance / elasticity
############################

Main running instructions
+++++++++++++++++++++++++

Before running you can specify :ref:`CCL_WORLD_SIZE` = N, where N - number of rank to start. If you use k8s with k8s manager support - N = replicasize by default.

You can specify your own function which will decide whether CCL should wait / use / finalize on "world" resize event
::
        typedef enum ccl_resize_action
        {
            // Wait additional changes number of ranks
            ccl_ra_wait     = 0,
            // Running with current number of ranks
            ccl_ra_use      = 1,
            // Finalize work
            ccl_ra_finalize = 2,
        } ccl_resize_action_t;

        typedef ccl_resize_action_t(*ccl_on_resize_fn_t)(size_t comm_size);

        ccl_set_resize_callback(ccl_on_resize_fn_t callback);
::

In this case CCL would be notified framework by this callback when changed number of ranks.

If you want to use only :ref:`CCL_WORLD_SIZE` or replicasize you can use:
::
        ccl_set_resize_callback(NULL);
::

If you don't call ccl_set_resize_callback function then CCL will be wait exactly :ref:`CCL_WORLD_SIZE` or replicasize ranks without fault tolerant/elasticity

Running N ranks in k8s without k8s manager, like set of pods
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

-   :ref:`CCL_PM_TYPE` = 1

-   :ref:`CCL_K8S_API_ADDR` = k8s server address and port, like IP:PORT

-   Run your example.



Running N ranks in k8s use statefulset / deployment as manager
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

-   :ref:`CCL_PM_TYPE` = 1

-   :ref:`CCL_K8S_API_ADDR` = k8s server address

-   :ref:`CCL_K8S_MANAGER_TYPE` = k8s

-   Run your example.

Running N ranks without mpirun
++++++++++++++++++++++++++++++

-   :ref:`CCL_PM_TYPE` = 1

-   :ref:`CCL_KVS_IP_EXCHANGE` = 1

-   :ref:`CCL_KVS_IP_PORT` = ip_port of one of your node where you run example

-   Run your example.