Caching of collective operations
********************************

Collective operations may have expensive initialization phase (e.g. allocation of internal structures/buffers, registration of memory buffers, handshake with peers, etc.). 
oneCCL amortizes these overheads by caching collective internal representations and reusing them on the subsequent calls.

To control this, set ``coll_attr.to_cache = 1`` and ``coll_attr.match_id = <match_id>``, where
``<match_id>`` is a unique string (e.g. tensor name). Note that:

- ``<match_id>`` should be the same for a specific collective operation across all ranks;
- if the same tensor is a part of different collective operations, ``match_id`` should have different values for each of these operations.
