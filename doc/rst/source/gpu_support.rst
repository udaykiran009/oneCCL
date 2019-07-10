GPU Support
===========

The choice between CPU and GPU backends is performed by specifying ``iccl_stream_type`` value when creating iccl stream object. For GPU backend one should specify ``iccl_stream_sycl`` there. For collective operations which operate on SyCL stream, CCL expects communication buffers to be ``sycl::buffer`` objects casted to ``void*``.
