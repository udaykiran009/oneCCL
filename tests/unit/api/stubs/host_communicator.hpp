#pragma once
#include <memory>

class ccl_comm;
namespace ccl
{
class host_communicator
{
public:
    host_communicator(std::shared_ptr<ccl_comm> impl);

    size_t rank() const;
    size_t size() const;

    void barrier_impl();
    std::shared_ptr<ccl_comm> comm_impl;
};
}
