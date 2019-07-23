Persistent collective operations
********************************

Collective operations may have expensive initialization phase (like allocation of internal structures/buffers, registration of memory buffers, handshake with peers, etc.).
oneAPI CCL amortizes these overheads by caching collective internal representation and reusing them on subsequent calls.

To control, use ``coll_attr.to_cache = 1``, which is enabled by default.
