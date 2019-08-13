Prioritization of collective operations
****************************************

oneAPI CCL supports prioritization of collective operations. Prioritization allows to change the order of operations execution and may help to postpone execution of non-urgent operations to complete urgent operations early.

The collective prioritization is controlled by priority value. Note that the priority must be a non-negative number where a higher number stands for a higher priority.

There are few prioritization modes:

-   None - default mode when all collective operations have the same priority.
-	Direct - priority is explicitly specified by users using ``coll_attr.priority``.
-	LIFO (Last In, First Out) - priority is implicitly increased on each collective calls. In this case user doesn't have to specify a priority.

The prioritization mode is controlled by :ref:`CCL_PRIORITY`.
