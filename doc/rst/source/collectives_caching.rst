Caching of collective operations
********************************

Collective operations may have expensive initialization phase (like allocation of internal structures/buffers, registration of memory buffers, handshake with peers, etc.). oneAPI CCL amortizes these overheads by caching collective internal representation and reusing them on subsequent calls.

To control, set ``coll_attr.to_cache = 1`` and ``coll_attr.match_id = <match_id>``.
``<match_id>`` is unique string, for example tensor name, which should be the same for specific collective operation across all ranks.

The same tensor can be part of different collective operations, for example bcast and allreduce.
In this case ``match_id`` should have different values for different operations.
