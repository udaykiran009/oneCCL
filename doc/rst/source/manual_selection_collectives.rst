Manual selection of collective algorithms
*****************************************

You can manually select a collective algorithm using :ref:`CCL_ALLREDUCE`:

-	``ring`` – reduce_scatter+allgather ring.
-	``ring_rma`` - reduce_scatter+allgather ring using rma communications.
-	``starlike`` – may be beneficial for imbalanced workloads.
-	``tree`` – Rabenseifner’s algorithm (**default**).

The default algorithm for small messages is recursive-doubling, for large messages - Rabenseifner’s.
