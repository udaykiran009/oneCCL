oneAPI Collective Communications Library
=========================================
   
oneAPI Collective Communications Library (oneCCL) is a library that provides an efficient implementation of
communication patterns used in deep learning. 

oneCCL features include:

- Built on top of lower-level communication middleware – MPI and libfabrics.
- Optimized to drive scalability of communication patterns by allowing to easily trade-off compute for communication performance.
- Enables a set of DL-specific optimizations, such as prioritization, persistent operations, out of order execution, etc.
- Works across various interconnects: Intel(R) Omni-Path Architecture, InfiniBand*, and Ethernet.
- Common API sufficient to support communication workflows within Deep Learning frameworks (Caffe*, nGraph*, Horovod*, etc.)

oneCCL package comprises the oneCCL Software Development Kit (SDK) and the Intel(R) MPI Library Runtime components.

Contents:
=========

.. toctree::
   :maxdepth: 1
   :caption: Get Started with oneCCL

   overview.rst
   prerequisites.rst
   sample.rst
   installation.rst
   sys_requirements.rst
   legal.rst
   optimization_notice.rst

.. toctree::
   :maxdepth: 1
   :caption: Programming Model

   spec/generic_workflow.rst
   spec/main_objects.rst
   spec/communication_primitives.rst
   spec/completion.rst
   spec/error_handling.rst
   spec/ccl_datatypes.rst
   spec/gpu_support.rst
   spec/cpu_support.rst

.. toctree::
   :maxdepth: 1
   :caption: General Configuration

   collectives_execution.rst
   transport_selection.rst
   
.. toctree::
   :maxdepth: 1
   :caption: Advanced Configuration

   collective_algorithms_selection.rst
   collectives_caching.rst
   collectives_prioritization.rst
   collectives_fusion.rst
   sparse_collectives.rst
   unordered_collectives.rst
   elasticity.rst

.. toctree::
   :maxdepth: 1
   :caption: Reference Materials

   env_variables.rst
   api/library_root.rst
