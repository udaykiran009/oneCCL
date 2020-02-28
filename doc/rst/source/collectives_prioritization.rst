Prioritization of collective operations
****************************************

|product_short| supports prioritization of collective operations that control the order in which individual collective operations are executed. 
This allows to postpone execution of non-urgent operations to complete urgent operations earlier, which may be beneficial for many use cases.

The collective prioritization is controlled by priority value. Note that the priority must be a non-negative number with a higher number standing for a higher priority.

There are the following prioritization modes:

-   None - default mode when all collective operations have the same priority.
-	Direct - you explicitly specify priority using ``coll_attr.priority``.
-	LIFO (Last In, First Out) - priority is implicitly increased on each collective call. In this case, you do not have to specify priority.

The prioritization mode is controlled by :ref:`CCL_PRIORITY`.
