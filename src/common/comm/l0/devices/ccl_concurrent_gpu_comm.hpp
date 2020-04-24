#pragma once
#include <map>
#include <memory>
#include <list>
#include <set>
#include <vector>

#include "common/comm/l0/devices/ccl_gpu_base_comm.hpp"

namespace native
{

//Adapter for different thread devices
template<class device_t>
class ccl_thread_comm : public ccl_gpu_base_comm<ccl_thread_comm<device_t>,
                                                 gpu_types::CONCURRENT_GPU + device_t::type_idx()>
{
public:
    using base = ccl_gpu_base_comm<ccl_thread_comm<device_t>,
                                   gpu_types::CONCURRENT_GPU + device_t::type_idx()>;
    using typename base::comm_rank_t;

    template<ccl_coll_type algo_type,
             ccl::device_topology_type topology_type>
    using gpu_module_t = typename device_t::template gpu_module_t<algo_type, topology_type>;    //same as in-process GPU

    template<ccl_coll_type algo_type,
             ccl::device_topology_type topology_type,
             class native_data_type>
    using gpu_kernel_t = typename gpu_module_t<algo_type, topology_type>::template kernel<native_data_type>;

    static constexpr const char* name_impl()
    {
        return "CONCURRENT_GPU";
    }

    ccl_thread_comm( ccl_device& assigned_device, typename base::comm_rank_t idx, device_t& next_thread_device)
      : base(assigned_device, idx),
        next_thread_gpu_comm(next_thread_device)
    {
    }

    ~ccl_thread_comm() = default;

    std::string to_string_impl() const
    {
        std::string ret(name_impl());
        ret = ret + "(" + next_thread_gpu_comm.to_string_impl() + ")";
        return ret;
    }

    template<ccl::device_topology_type topology_type>
    topology_addr<topology_type> get_comm_data() const
    {
        return next_thread_gpu_comm.template get_comm_data<topology_type>();
    }

    template<ccl_coll_type module_type,
             ccl::device_topology_type topology_type,
             class native_data_type>
    gpu_kernel_t<module_type, topology_type, native_data_type>& get_gpu_kernel()
    {
        return next_thread_gpu_comm.template get_gpu_kernel<module_type, topology_type, native_data_type>();
    }

private:
    device_t& next_thread_gpu_comm;
};
}
