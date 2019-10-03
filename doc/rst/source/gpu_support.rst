GPU support
===========

The choice between CPU and GPU backends is performed by specifying ``ccl_stream_type`` value at the moment of creating ccl stream object.
For GPU backend one should specify ``ccl_stream_sycl`` as the first argument. For collective operations, which operate on SYCL* stream, oneCCL expects communication buffers to be ``sycl::buffer`` objects casted to ``void*``.

The example below demonstrates these concepts.

.. rubric:: Example

Conisider simple allreduce example for GPU. As a first step, GPU ccl stream object is created:

**C version of oneCCL API:**

::

    ccl_stream_create(ccl_stream_sycl, &q, &stream);

**C++ version of oneCCL API:**

::

    ccl::stream_t stream = ccl::environment::instance().create_stream(cc::stream_type::sycl, &q);

``q`` is an object of type ``sycl::queue``.

To illustrate the ``ccl_allreduce`` execution, initialize ``sendbuf`` (in real scenario it is provided by application):

::

    auto host_acc_sbuf = sendbuf.get_access<mode::write>();
    for (i = 0; i < COUNT; i++) {
        host_acc_sbuf[i] = rank;
    }

For demostration purposes only, modify the ``sendbuf`` on the GPU side:

::

    q.submit([&](cl::sycl::handler& cgh) {
        auto dev_acc_sbuf = sendbuf.get_access<mode::write>(cgh);
        cgh.parallel_for<class allreduce_test_sbuf_modify>(range<1>{COUNT}, [=](item<1> id) {
            dev_acc_sbuf[id] += 1;
        });
    });

``ccl_allreduce`` invocation performs reduction of values from all processes and distributes the result to all processes.
For this case, the result is an array with the size equal to the number of processes (#processes),
where all elements are equal to (#processes * (#processes - 1) / 2) since it represents the sum of arithmetical progression.

**C version of oneCCL API:**

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

**C++ version of oneCCL API:**

::

    comm.allreduce(sendbuf,
                   recvbuf,
                   COUNT,
                   ccl::data_type::dt_int,
                   ccl::reduction::sum,
                   nullptr, /* attr */
                   stream)->wait();

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

When using C version of oneCCL API, it is required to explicitly free the created GPU ccl stream object:

::

    ccl_stream_free(stream);

For C++ version of oneCCL API this will be performed implicitly.
