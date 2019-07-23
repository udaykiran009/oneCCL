CPU Support
===========

The choice between CPU and GPU backends is performed by specifying ``ccl_stream_type`` value when creating ccl stream object. For CPU backend one should specify ``ccl_stream_cpu`` there. For collective operations performed using CPU stream, CCL expect communication buffers to reside in the host memory. 
