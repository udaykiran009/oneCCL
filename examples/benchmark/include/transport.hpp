#pragma once

#include <map>
#include <vector>

#include "oneapi/ccl.hpp"

class transport_data {

public:

    static transport_data& instance();
    static size_t get_comm_size();

    int get_rank() const noexcept;
    int get_size() const noexcept;

    ccl::shared_ptr_class<ccl::kvs> get_kvs();

    void init_host_comms(size_t ranks_per_proc);
    std::vector<ccl::communicator>& get_host_comms();

    std::vector<ccl::stream>& get_streams();

#ifdef CCL_ENABLE_SYCL
    void init_device_comms(size_t ranks_per_proc);
    std::vector<ccl::communicator>& get_device_comms();
#endif /* CCL_ENABLE_SYCL */

private:

    transport_data();
    ~transport_data();

    int rank;
    int size;

    std::vector<size_t> local_ranks;
    ccl::shared_ptr_class<ccl::kvs> kvs;
    std::vector<ccl::communicator> host_comms;
    std::vector<ccl::stream> streams;

#ifdef CCL_ENABLE_SYCL
    std::vector<ccl::communicator> device_comms;
#endif /* CCL_ENABLE_SYCL */

    void init_by_mpi();
    void deinit_by_mpi();
};
