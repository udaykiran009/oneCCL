#pragma once
#include <memory>

class ccl_comm;
struct host_communicator
{
    host_communicator(std::shared_ptr<ccl_comm> impl);

    size_t rank() const;
    size_t size() const;

    void barrier();
    std::shared_ptr<ccl_comm> comm_impl;
};
