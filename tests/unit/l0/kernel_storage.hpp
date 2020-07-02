#include <iostream>
#include <future>
#include <list>
#include <map>
#include <thread>
#include <stdexcept>
#include <vector>

#include "utils.hpp"
#include "common/comm/l0/modules/modules_source_data.hpp"
using test_storage = native::modules_src_container<ccl_coll_allgatherv,ccl_coll_allreduce>;

TEST(MODULES, loading_kernels)
{
    {
        std::ofstream myfile;
        myfile.open ("example.cl");
        myfile << "testfile\n";
    }
    test_storage::instance().load_kernel_source<ccl_coll_allgatherv>("example.cl", ccl::device_topology_type::ring);

    bool success = false;
    try
    {
        test_storage::instance().load_kernel_source<ccl_coll_allgatherv>("example.cl", ccl::device_topology_type::ring);
    }
    catch(const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
        success = true;
    }
    if (!success)
    {
        assert(success && "Add duplicated file is not allowed");
        abort();
    }

    test_storage::instance().load_kernel_source<ccl_coll_allgatherv>("example.cl", ccl::device_topology_type::a2a);
}
