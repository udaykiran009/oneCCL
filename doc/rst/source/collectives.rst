Collective Operations
=====================

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
