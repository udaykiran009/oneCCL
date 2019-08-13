GPU support
===========

The choice between CPU and GPU backends is performed by specifying ``ccl_stream_type`` value when creating ccl stream object. For GPU backend one should specify ``ccl_stream_sycl`` as the first argument. For collective operations which operate on SyCL stream, CCL expects communication buffers to be ``sycl::buffer`` objects casted to ``void*``.
To demonstrate these concepts let us concider the allreduce sample. As a first step GPU ccl stream object is created:

C version of CCL API

::

    ccl_stream_create(ccl_stream_sycl, &q, &stream);

C++ version of CCL API

::

    ccl::stream stream(cc::stream_type::sycl, &q);

``q`` is an object of type ``sycl::queue``.

To illustrate the ``ccl_allreduce`` execution we need to initialize ``sendbuf`` (in real scenario it is provided by application):

::

    auto host_acc_sbuf = sendbuf.get_access<mode::write>();
    for (i = 0; i < COUNT; i++) {
        host_acc_sbuf[i] = rank;
    }

Then on the GPU side we modify the ``sendbuf``. This is for demostration purposes only.

::

    q.submit([&](cl::sycl::handler& cgh) {
        auto dev_acc_sbuf = sendbuf.get_access<mode::write>(cgh);
        cgh.parallel_for<class allreduce_test_sbuf_modify>(range<1>{COUNT}, [=](item<1> id) {
            dev_acc_sbuf[id] += 1;
        });
    });

``ccl_allreduce`` invocation performs reduction of values from all processes and distributes the result to all processes. For this case the result is an array of #processes size with all elements equal to #processes*(#processes+1)/2) since it represents the sum of arithmetical progression.

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

Check the correctness of ``ccl_allreduce`` on the GPU:

::

    q.submit([&](handler& cgh) {
        auto dev_acc_rbuf = recvbuf.get_access<mode::write>(cgh);
        cgh.parallel_for<class allreduce_test_rbuf_check>(range<1>{COUNT}, [=](item<1> id) {
            if (dev_acc_rbuf[id] != size*(size+1)/2) {
              dev_acc_rbuf[id] = -1;
            }
        });
    });

::

    if (rank == COLL_ROOT) {
        auto host_acc_rbuf_new = recvbuf.get_access<mode::read>();
        for (i = 0; i < COUNT; i++) {
            if (host_acc_rbuf_new[i] == -1) {
                cout << "FAILED" << endl;
                break;
            }
        }
        if (i == COUNT) {
            cout<<"PASSED"<<endl;
        }
    }

When using C version of CCL API it is required to explicitly free the created GPU ccl stream object:

::

    ccl_stream_free(stream);

For C++ version of CCL API this will be performed implicitly.
