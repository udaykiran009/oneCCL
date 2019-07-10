CPU Support
===========

The choice between CPU and GPU backends is performed by specifying ``iccl_stream_type`` value when creating iccl stream object. For CPU backend one should specify ``iccl_stream_cpu`` there. For collective operations performed using CPU stream, CCL expect communication buffers to reside in the host memory. 
