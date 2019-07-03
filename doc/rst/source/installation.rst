   .. highlight:: bash

.. highlight:: bash 

Installation Guide
==================

This page explains how to install and configure the Intel® Collective Communications Library (ICCL). 
Intel(R) CCL supports a number of installation scenarios:

* Installation using CLI 
* RPM-based installation
* Installation using tar.gz.


Installation Through Command Line Interface (CLI)
*************************************************

To install Intel® CCL using CLI, follow these steps:

#. Go to the ``iccl`` folder:

   ::

      cd iccl

#. Create a new folder:

   ::
   
      mkdir build

#. Go to the folder created:
   
   :: 
   
      cd build

#. Launch CMake:
   
   ::
   
      cmake ..

#. Install the product:
   
   ::
   
      make -j install

In order to have a clear build, create a new ``build`` directory and invoke ``cmake`` within the directory.

Custom Installation
^^^^^^^^^^^^^^^^^^^

You may customize CLI-based installation and specify, for example, directory, compiler, and build type.

Specify Installation Directory
##############################

Modify the ``cmake`` command:

::
   
   cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/installation/directory

If no ``-DCMAKE_INSTALL_PREFIX`` is specified, ICCL is installed into the ``_install`` subdirectory of the current build directory. For example, ``iccl/build/_install``


Specify compiler
################

Modify the ``cmake`` command:

.. code-block::
   
   cmake .. -DCMAKE_C_COMPILER=your_c_compiler -DCMAKE_CXX_COMPILER=your_cxx_compiler

Specify Build Type
##################

Modify the ``cmake`` command:

.. code-block::        
   
   cmake .. -DCMAKE_BUILD_TYPE=[Debug|Release|RelWithDebInfo|MinSizeRel]

Enable ``make`` Verbose Output
##############################

Modify the ``make`` command as follows to see all parameters used by ``make`` during compilation and linkage:

.. code-block::      

   make -j VERBOSE=1

Archive Installed Files
#######################

.. code-block::    

   make -j install
   make archive

Build with Address Sanitizer
############################

Modify the ``cmake`` command as follow:

.. code-block::        

   cmake .. -DCMAKE_BUILD_TYPE=Debug -DWITH_ASAN=true

**Note:** Address sanitizer only works in the debug build.

Make sure that libasan.so exists. :guilabel:`Add to prerequisites`.

Binary releases are available on our release page.

Installation using tar.gz
*************************

To install Intel® CCL using the tar.gz file in a user mode, execute the following commands:

.. code-block::
   
   $ tar zxf l_iccl-devel-64-<version>.<update>.<package#>.tgz

   $ cd l_iccl_<version>.<update>.<package#>

   $ ./install.sh

There is no uninstall script. To uninstall Intel® CCL, delete the whole installation directory.

Installation using RPM
**********************

Intel® CCL is available through the RPM Package Manager. To install the library in a root mode using RPM, follow these steps:

#. Log in as root.

#. Install the following package:

   .. code-block:: 

      $ rpm -i intel-iccl-devel-64-<version>.<update>-<package#>.x86_64.rpm
   
   where ``<version>.<update>-<package#>`` is a string. For example, ``2017.0-009``.

To uninstall Intel® CCL using the RPM Package Manager, execute this command:

.. code-block:: 

   $ rpm -e intel-iccl-devel-64-<version>.<update>-<package#>.x86_64
