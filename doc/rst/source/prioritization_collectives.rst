Prioritization for collective operations
****************************************

oneAPI CCL supports prioritization of collective operations. Prioritization allows to control the **speed (?)** of operation execution.

Note that the priority must be a non-negative number where a higher number stands for a higher priority.

There are two prioritization modes:

-	Direct - priority is explicitly specified by users using ``coll_attr.priority``.
-	LIFO (Last In, First Out) - priority is implicitly increased on each collective calls. User do not specify a priority.

To control the mode, pass ``none``, ``direct``, ``lifo`` to the :ref:`CCL_PRIORITY` environment variable. By default, prioritization is disabled (``none``). 
