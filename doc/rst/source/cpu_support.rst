CPU support
===========

The choice between CPU and GPU backends is performed by specifying ``ccl_stream_type`` value when creating ccl stream object. For CPU backend one should specify ``ccl_stream_cpu`` there. For collective operations performed using CPU stream, CCL expect communication buffers to reside in the host memory. 

To demonstrate these concepts let us conisider simple allreduce example for CPU. As a first step CPU ccl stream object is created:

C version of CCL API

::

    /* For CPU stream we pass NULL instead of native stream pointer */
    ccl_stream_create(ccl_stream_cpu, NULL, &stream);

C++ version of CCL API

::

    /* For CPU we pass NULL instead of native stream pointer */
    ccl::stream stream(cc::stream_type::cpu, NULL);

or just

::

    ccl::stream stream;

To illustrate the ``ccl_allreduce`` execution we need to initialize ``sendbuf`` (in real scenario it is supplied by application):

::

    /* initialize sendbuf */
    for (i = 0; i < COUNT; i++) {
        sendbuf[i] = rank;
    }


``ccl_allreduce`` invocation performs reduction of values from all processes and distributes the result to all processes. For this case the result is an array of #processes size with all elements equal to #processes*(#processes-1)/2) since it represents the sum of arithmetical progression.

C version of CCL API

::

    ccl_allreduce(&sendbuf,
                  &recvbuf,
                  COUNT,
                  ccl_dtype_int,
                  ccl_reduction_sum,
                  NULL, /* attr */
                  NULL, /* comm */
                  stream,
                  &request);
    ccl_wait(request);

C++ version of CCL API

::

    comm.allreduce(&sendbuf,
                   &recvbuf,
                   COUNT,
                   ccl::data_type::dt_int,
                   ccl::reduction::sum,
                   nullptr, /* attr */
                   &stream)->wait();

When using C version of CCL API it is required to explicitly free ccl stream object:

::

    ccl_stream_free(stream);

For C++ version of CCL API this will be performed implicitly.

