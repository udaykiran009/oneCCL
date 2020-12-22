#pragma once

#include <map>
#include <vector>

#include "oneapi/ccl.hpp"
#include "types.hpp"

class transport_data {
public:
    static transport_data& instance();
    static size_t get_comm_size();

    int get_rank() const noexcept;
    int get_size() const noexcept;

    ccl::shared_ptr_class<ccl::kvs> get_kvs();
    ccl::communicator& get_service_comm();
    void init_comms(user_options_t& options);
    std::vector<ccl::communicator>& get_comms();
    void reset_comms();

    std::vector<ccl::stream>& get_streams();
    std::vector<ccl::stream>& get_bench_streams();

private:
    transport_data();
    ~transport_data();

    int rank;
    int size;

    std::vector<size_t> local_ranks;

    ccl::shared_ptr_class<ccl::kvs> kvs;
    std::vector<ccl::communicator> service_comms;
    std::vector<ccl::communicator> comms;

    /*
       FIXME: explicitly separate CCL and bench streams
              while runtime doesn't provide MT on the same queue
    */
    std::vector<ccl::stream> streams;
    std::vector<ccl::stream> bench_streams;

    void init_by_mpi();
    void deinit_by_mpi();
};
