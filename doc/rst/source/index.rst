Intel® Machine Learning Scaling Library
=========================================
   
Intel® oneAPI Collective Communications Library (Intel® CCL) is a library that provides an efficient implementation of
communication patterns used in deep learning. 

The most popular features are:

- Built on top of lower-level communication middleware – MPI and libfabrics.
- Optimized to drive scalability of communication patterns by allowing to easily trade-off compute for communication performance.
- Enables a set of DL-specific optimizations, such as prioritization, persistent operations, out of order execution, etc.
- Works across various interconnects: Intel® Omni-Path Architecture, InfiniBand*, and Ethernet.
- Common API that supports Deep Learning frameworks (Caffe*, nGraph*, Horovod*, etc.)

Intel® CCL package comprises the Intel CCL Software Development Kit (SDK) and the Intel® MPI Library Runtime components.

Contents:
=========

.. toctree::
   :maxdepth: 1
   :caption: Get Started with Intel® CCL

   overview.rst
   prerequisites.rst
   launch_sample_app.rst
   installation.rst
   sys_requirements.rst
   legal.rst


.. toctree::
   :maxdepth: 1
   :caption: Programming Model

   generic_workflow.rst
   dl_frameworks-support.rst
   gpu_support.rst
   cpu_support.rst

.. toctree::
   :maxdepth: 1
   :caption: General Configuration

   affinity.rst
   persistent_collectives.rst
   fabric_selection.rst
   
.. toctree::
   :maxdepth: 1
   :caption: Advanced Configuration

   prioritization_collectives.rst
   manual_selection_collectives.rst
   kubernetes.rst
   out_of_order.rst

.. toctree::
   :maxdepth: 1
   :caption: Reference Materials

   variables.rst
   api_reference.rst   

