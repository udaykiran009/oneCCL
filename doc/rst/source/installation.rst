.. highlight:: bash

Installation guide
==================

This page explains how to install and configure the oneAPI Collective Communications Library (oneCCL). 
oneCCL supports a number of installation scenarios:

* Installation using CLI 
* RPM-based installation
* Installation using tar.gz


Installation Through Command Line Interface (CLI)
*************************************************

To install oneCCL using CLI, follow these steps:

#. Go to the ``ccl`` folder:

   ::

      cd ccl

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

If no ``-DCMAKE_INSTALL_PREFIX`` is specified, oneCCL is installed into the ``_install`` subdirectory of the current build directory. For example, ``ccl/build/_install``


Specify compiler
################

Modify the ``cmake`` command:

::

   cmake .. -DCMAKE_C_COMPILER=your_c_compiler -DCMAKE_CXX_COMPILER=your_cxx_compiler

Specify Build Type
##################

Modify the ``cmake`` command:

::

   cmake .. -DCMAKE_BUILD_TYPE=[Debug|Release|RelWithDebInfo|MinSizeRel]

Enable ``make`` Verbose Output
##############################

To see all parameters used by ``make`` during compilation and linkage, modify the ``make`` command as follows:

::

   make -j VERBOSE=1

Archive Installed Files
#######################

::

   make -j install

Build with Address Sanitizer
############################

Modify the ``cmake`` command as follow:

::

   cmake .. -DCMAKE_BUILD_TYPE=Debug -DWITH_ASAN=true

..note:: 

  Address sanitizer only works in the debug build.

Make sure that libasan.so exists. :guilabel:`Add to prerequisites`.

Binary releases are available on our release page.

Installation using tar.gz
*************************

To install oneCCL using the tar.gz file in a user mode, execute the following commands:

.. prompt:: bash

   tar zxf l_ccl-devel-64-<version>.<update>.<package#>.tgz
   cd l_ccl_<version>.<update>.<package#>
   ./install.sh

There is no uninstall script. To uninstall oneCCL, delete the whole installation directory.

Installation using RPM
**********************

oneCCL is available through the RPM Package Manager. To install the library in a root mode using RPM, follow these steps:

#. Log in as root.

#. Install the following package:

.. prompt:: bash

   rpm -i intel-ccl-devel-64-<version>.<update>-<package#>.x86_64.rpm
   
   where ``<version>.<update>-<package#>`` is a string. For example, ``2017.0-009``.

To uninstall oneCCL using the RPM Package Manager, execute this command:

.. prompt:: bash

   rpm -e intel-ccl-devel-64-<version>.<update>-<package#>.x86_64
