#pragma once

#include <map>
#include <vector>

#include "base.hpp"
#include "oneapi/ccl.hpp"
#ifdef CCL_ENABLE_SYCL
#include "sycl_base.hpp"
#endif /* CCL_ENABLE_SYCL */

class transport_data {
public:
    static transport_data& instance();

    void init_comms();
    void reset_comms();

    int get_rank() const noexcept;
    int get_size() const noexcept;

    ccl::shared_ptr_class<ccl::kvs> get_kvs();
    ccl::communicator& get_comm();
    ccl::communicator& get_service_comm();
    ccl::stream& get_stream();

#ifdef CCL_ENABLE_SYCL
    buf_allocator<char>& get_allocator();
#endif /* CCL_ENABLE_SYCL */

private:
    transport_data();
    ~transport_data();

    void init_by_mpi();
    void deinit_by_mpi();

    int rank;
    int size;

    ccl::shared_ptr_class<ccl::kvs> kvs;
    std::vector<ccl::communicator> comms;
    std::vector<ccl::communicator> service_comms;
    std::vector<ccl::stream> streams;

#ifdef CCL_ENABLE_SYCL
    std::vector<buf_allocator<char>> allocators;
#endif /* CCL_ENABLE_SYCL */

    const int ranks_per_proc = 1;
};
