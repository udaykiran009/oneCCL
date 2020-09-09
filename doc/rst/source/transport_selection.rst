.. _mpi: https://software.intel.com/content/www/us/en/develop/tools/mpi-library.html

Transport selection
===================

|product_short| supports two transports for inter-node communication: |mpi|_ and `libfabrics <https://github.com/ofiwg/libfabric>`_.

The transport selection is controlled by :ref:`CCL_ATL_TRANSPORT`.

In case of MPI over libfaric implementation (for example, Intel\ |reg|\  MPI Library 2019) or in case of direct libfabric transport, 
the selection of specific libfaric provider is controlled by the ``FI_PROVIDER`` environment variable.
