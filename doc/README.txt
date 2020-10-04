-------------------------------------------------------
oneAPI Collective Communications Library for Linux* OS
README
-------------------------------------------------------

----------------------
License Acceptance
----------------------

Review the file cclEULA.txt. To accept the End User License Agreement,
unzip the included file package.zip to a temporary directory with
the following command:
 
> unzip  package.zip

and type "accept" when prompted.

------------
Introduction
------------

oneAPI Collective Communications Library (oneCCL) is a library providing
an efficient implementation of communication patterns used in deep learning.

    - Built on top of MPI, allows for use of other communication libraries
    - Optimized to drive scalability of communication patterns
    - Works across various interconnects: Intel(R) Omni-Path Architecture,
      InfiniBand*, and Ethernet
    - Common API to support Deep Learning frameworks (Caffe*, Theano*,
      Torch*, etc.)

oneCCL package comprises the oneCL Software Development Kit (SDK)
and the Intel(R) MPI Library Runtime components.

----------------------------------------------------
Installing oneAPI Collective Communications Library
----------------------------------------------------

Installing oneCCL using install.sh (user mode):

 Run install.sh and follow the instructions.

 There is no uninstall script. To uninstall oneCCL, delete the entire
 directory where you have installed the package.

-------------------
Directory Structure
-------------------

Following a successful installation, the files associated with the oneCCL
are installed on your host system. The following directory map indicates the 
default structure and identifies the file types stored in each sub-directory:

`-- opt
    `-- intel        Common directory for Intel(R) Software Development Products.
            `-- iccl_<version>.<update>.<package#>
                |               Subdirectory for the version, specific update
                |               and package number of oneCCL.
				|-- env                                                                                                
				|   `-- vars.sh                                                                                        
				|-- examples                                                                                           
				|   |-- benchmark                                                                                      
				|   |   |-- CMakeLists.txt                                                                             
				|   |   |-- include                                                                                    
				|   |   |   |-- benchmark.hpp                                                                          
				|   |   |   |-- coll.hpp                                                                               
				|   |   |   |-- config.hpp                                                                             
				|   |   |   |-- cpu_coll.hpp                                                                           
				|   |   |   `-- sycl_coll.hpp                                                                          
				|   |   `-- src                                                                                        
				|   |       |-- allgatherv                                                                             
				|   |       |   |-- allgatherv_strategy.hpp                                                            
				|   |       |   |-- cpu_allgatherv_coll.hpp                                                            
				|   |       |   `-- sycl_allgatherv_coll.hpp                                                           
				|   |       |-- allreduce                                                                              
				|   |       |   |-- allreduce_strategy.hpp                                                             
				|   |       |   |-- cpu_allreduce_coll.hpp                                                             
				|   |       |   `-- sycl_allreduce_coll.hpp                                                            
				|   |       |-- alltoall                                                                               
				|   |       |   |-- alltoall_strategy.hpp                                                              
				|   |       |   |-- cpu_alltoall_coll.hpp                                                              
				|   |       |   `-- sycl_alltoall_coll.hpp                                                             
				|   |       |-- alltoallv                                                                              
				|   |       |   |-- alltoallv_strategy.hpp                                                             
				|   |       |   |-- cpu_alltoallv_coll.hpp                                                             
				|   |       |   `-- sycl_alltoallv_coll.hpp                                                            
				|   |       |-- bcast                                                                                  
				|   |       |   |-- bcast_strategy.hpp                                                                 
				|   |       |   |-- cpu_bcast_coll.hpp                                                                 
				|   |       |   `-- sycl_bcast_coll.hpp                                                                
				|   |       |-- benchmark.cpp                                                                          
				|   |       |-- declarations.hpp                                                                       
				|   |       |-- reduce                                                                                 
				|   |       |   |-- cpu_reduce_coll.hpp                                                                
				|   |       |   |-- reduce_strategy.hpp                                                                
				|   |       |   `-- sycl_reduce_coll.hpp                                                               
				|   |       `-- sparse_allreduce                                                                       
				|   |           |-- cpu_sparse_allreduce_coll.hpp                                                      
				|   |           |-- sparse_allreduce_base.hpp                                                          
				|   |           |-- sparse_allreduce_strategy.hpp                                                      
				|   |           |-- sparse_detail.hpp                                                                  
				|   |           `-- sycl_sparse_allreduce_coll.hpp                                                     
				|   |-- CMakeLists.txt                                                                                 
				|   |-- common                                                                                         
				|   |   |-- version.cpp                                                                                
				|   |   `-- CMakeLists.txt                                                                             
				|   |-- cpu                                                                                            
				|   |   |-- allgatherv.cpp                                                                             
				|   |   |-- allgatherv_cpp.cpp                                                                         
				|   |   |-- allgatherv_iov.cpp                                                                         
				|   |   |-- allreduce.c                                                                                
				|   |   |-- allreduce_cpp.cpp                                                                          
				|   |   |-- alltoall.c                                                                                 
				|   |   |-- alltoallv.cpp                                                                              
				|   |   |-- bcast.cpp                                                                                  
				|   |   |-- bcast_cpp.cpp                                                                              
				|   |   |-- CMakeLists.txt                                                                             
				|   |   |-- communicator.cpp                                                                           
				|   |   |-- cpu_allgatherv_test.c                                                                      
				|   |   |-- cpu_allreduce_bf16.c                                                                      
				|   |   |-- cpu_allreduce_test.cpp                                                                 
				|   |   |-- cpu_allreduce_test.c                                                                       
				|   |   |-- custom_allreduce.cpp                                                                       
				|   |   |-- datatype.cpp                                                                               
				|   |   |-- priority_allreduce.cpp                                                                     
				|   |   |-- reduce.cpp                                                                                 
				|   |   |-- reduce_cpp.cpp                                                                             
				|   |   |-- sparse_allreduce.cpp                                                                       
				|   |   |-- sparse_test_algo.hpp                                                                       
				|   |   `-- unordered_allreduce.cpp                                                                    
				|   |-- include                                                                                        
				|   |   |-- base.h                                                                                     
				|   |   |-- base.hpp                                                                                   
				|   |   |-- base_utils.hpp                                                                             
				|   |   |-- bf16.h                                                                                    
				|   |   `-- sycl_base.hpp                                                                              
				|   `-- sycl                                                                                           
				|       |-- CMakeLists.txt                                                                             
				|       |-- sycl_allgatherv_test.cpp                                                               
				|       |-- sycl_allgatherv_usm_test.cpp                                                                   
				|       |-- sycl_allreduce_test.cpp                                                                
				|       |-- sycl_alltoall_test.cpp                                                                 
				|       |-- sycl_alltoall_usm_test.cpp                                                                     
				|       |-- sycl_alltoallv_test.cpp                                                                
				|       |-- sycl_alltoallv_usm_test.cpp                                                                    
				|       |-- sycl_broadcast_test.cpp                                                                    
				|       |-- sycl_broadcast_usm_test.cpp                                                                        
				|       |-- sycl_reduce_test.cpp                                                                   
				|-- include                                                                                            
				|   |-- cpu_gpu_dpcpp                                                                                  
				|   |   |-- ccl_config.h                                                                               
				|   |   |-- ccl_device_type_traits.hpp                                                                 
				|   |   |-- ccl.h                                                                                      
				|   |   |-- ccl.hpp                                                                                    
				|   |   |-- ccl_types.h                                                                                
				|   |   |-- ccl_types.hpp                                                                              
				|   |   `-- ccl_type_traits.hpp                                                                        
				|   `-- cpu_icc                                                                                        
				|       |-- ccl_config.h                                                                               
				|       |-- ccl_device_type_traits.hpp                                                                 
				|       |-- ccl.h                                                                                      
				|       |-- ccl.hpp                                                                                    
				|       |-- ccl_types.h                                                                                
				|       |-- ccl_types.hpp                                                                              
				|       `-- ccl_type_traits.hpp                                                                        
				|-- lib                                                                                                
				|   |-- cpu_gpu_dpcpp                                                                                  
				|   |   |-- libccl.a                                                                                   
				|   |   |-- libccl_atl_mpi.a                                                                           
				|   |   |-- libccl_atl_mpi.so -> libccl_atl_mpi.so.1.0                                                 
				|   |   |-- libccl_atl_mpi.so.1                                                                        
				|   |   |-- libccl_atl_mpi.so.1.0 -> libccl_atl_mpi.so.1                                               
				|   |   |-- libccl_atl_ofi.a                                                                           
				|   |   |-- libccl_atl_ofi.so -> libccl_atl_ofi.so.1.0                                                 
				|   |   |-- libccl_atl_ofi.so.1                                                                        
				|   |   |-- libccl_atl_ofi.so.1.0 -> libccl_atl_ofi.so.1                                               
				|   |   |-- libccl.so                                                                                  
				|   |   |-- libpmi.a                                                                                   
				|   |   |-- libpmi.so -> libpmi.so.1.0                                                                 
				|   |   |-- libpmi.so.1                                                                                
				|   |   |-- libpmi.so.1.0 -> libpmi.so.1                                                               
				|   |   |-- libresizable_pmi.a                                                                         
				|   |   |-- libresizable_pmi.so -> libresizable_pmi.so.1.0                                             
				|   |   |-- libresizable_pmi.so.1                                                                      
				|   |   `-- libresizable_pmi.so.1.0 -> libresizable_pmi.so.1                                           
				|   `-- cpu_icc                                                                                        
				|       |-- libccl.a                                                                                   
				|       |-- libccl_atl_mpi.a                                                                           
				|       |-- libccl_atl_mpi.so -> libccl_atl_mpi.so.1.0                                                 
				|       |-- libccl_atl_mpi.so.1
				|       |-- libccl_atl_mpi.so.1.0 -> libccl_atl_mpi.so.1
				|       |-- libccl_atl_ofi.a
				|       |-- libccl_atl_ofi.so -> libccl_atl_ofi.so.1.0
				|       |-- libccl_atl_ofi.so.1
				|       |-- libccl_atl_ofi.so.1.0 -> libccl_atl_ofi.so.1
				|       |-- libccl.so
				|       |-- libpmi.a
				|       |-- libpmi.so -> libpmi.so.1.0
				|       |-- libpmi.so.1
				|       |-- libpmi.so.1.0 -> libpmi.so.1
				|       |-- libresizable_pmi.a
				|       |-- libresizable_pmi.so -> libresizable_pmi.so.1.0
				|       |-- libresizable_pmi.so.1
				|       `-- libresizable_pmi.so.1.0 -> libresizable_pmi.so.1
				|-- licensing
				|   |-- license.txt
				|   `-- third-party-programs.txt
				`-- modulefiles
					`-- ccl


--------------------------------
Disclaimer and Legal Information
--------------------------------
No license (express or implied, by estoppel or otherwise) to any intellectual
property rights is granted by this document.

Intel disclaims all express and implied warranties, including without limitation,
the implied warranties of merchantability, fitness for a particular purpose, and
non-infringement, as well as any warranty arising from course of performance,
course of dealing, or usage in trade.

This document contains information on products, services and/or processes in
development. All information provided here is subject to change without notice.
Contact your Intel representative to obtain the latest forecast, schedule,
specifications and roadmaps.

The products and services described may contain defects or errors known as
errata which may cause deviations from published specifications. Current
characterized errata are available on request.

No computer software can provide absolute security. End users are responsible for
securing their own deployment of computer software in any environment.

Intel, Intel Core, Xeon, Xeon Phi and the Intel logo are trademarks of Intel
Corporation in the U.S. and/or other countries.

* Other names and brands may be claimed as the property of others.

(C) Intel Corporation.

Optimization Notice
-------------------

Intel's compilers may or may not optimize to the same degree for non-Intel
microprocessors for optimizations that are not unique to Intel microprocessors.
These optimizations include SSE2, SSE3, and SSSE3 instruction sets and other
optimizations. Intel does not guarantee the availability, functionality, or
effectiveness of any optimization on microprocessors not manufactured by Intel.
Microprocessor-dependent optimizations in this product are intended for use 
with Intel microprocessors. Certain optimizations not specific to Intel 
microarchitecture are reserved for Intel microprocessors. Please refer to the 
applicable product User and Reference Guides for more information regarding the
specific instruction sets covered by this notice.

Notice revision #20110804
