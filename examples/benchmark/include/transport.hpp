#pragma once
#include "base_utils.hpp"


class transport_settings
{
public:
    static transport_settings& instance();
    const int get_rank() const noexcept;
    const int get_size() const noexcept;

    ccl::shared_ptr_class<ccl::kvs> get_kvs();
private:
    transport_settings();
    ~transport_settings();

    int rank;
    int size;
    ccl::shared_ptr_class<ccl::kvs> kvs;
    void init_by_mpi();
    void deinit_by_mpi();
};
