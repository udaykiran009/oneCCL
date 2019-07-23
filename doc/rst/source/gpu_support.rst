GPU Support
===========

The choice between CPU and GPU backends is performed by specifying ``ccl_stream_type`` value when creating ccl stream object. For GPU backend one should specify ``ccl_stream_sycl`` there. For collective operations which operate on SyCL stream, CCL expects communication buffers to be ``sycl::buffer`` objects casted to ``void*``.
