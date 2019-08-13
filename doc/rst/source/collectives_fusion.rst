Fusion of collective operations
*******************************

In some cases it may be benefial to postpone execution of collective operations and execute them later together as single operation in batch mode. This can hide per operation setup overhead and improve interconnect saturation. oneAPI CCL provides such fusion functionality.

The fusion is enabled by :ref:`CCL_FUSION`.

The advanced configuration is controlled by :ref:`CCL_FUSION_BYTES_THRESHOLD`, :ref:`CCL_FUSION_COUNT_THRESHOLD` and :ref:`CCL_FUSION_CYCLE_MS`.
