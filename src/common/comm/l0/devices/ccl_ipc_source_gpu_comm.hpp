#pragma once
#include <map>
#include <memory>
#include <list>
#include <set>
#include <vector>

#include "common/comm/l0/devices/ccl_gpu_base_comm.hpp"
#include "common/comm/l0/devices/proxy_observer_types.hpp"
#include "common/comm/l0/context/scaling_ctx/ipc_session_key.hpp"

#include "common/comm/l0/devices/communication_structs/ipc_client.hpp"
namespace native {

//Adapter for different thread devices
template <class device_t>
class ccl_ipc_source_gpu_comm : public ccl_gpu_base_comm<ccl_ipc_source_gpu_comm<device_t>,
                                                         gpu_types::IPC_GPU + device_t::type_idx()>,
                                public proxy_multiple_observer<ccl_ipc_source_gpu_comm<device_t>,
                                                               std::nullptr_t,
                                                               std::nullptr_t,
                                                               process_group_context>,
                                public net::ipc_client {
public:
    using base = ccl_gpu_base_comm<ccl_ipc_source_gpu_comm<device_t>,
                                   gpu_types::IPC_GPU + device_t::type_idx()>;

    using proxy_base = proxy_multiple_observer<ccl_ipc_source_gpu_comm<device_t>,
                                               std::nullptr_t,
                                               std::nullptr_t,
                                               process_group_context>;
    using typename base::comm_rank_t;
    using impl_t = device_t;
    template <ccl_coll_type algo_type, ccl::group_split_type group, ccl::device_topology_type mode>
    using gpu_module_t =
        typename device_t::template gpu_module_t<algo_type, group, mode>; //same as in-process GPU

    template <ccl_coll_type algo_type,
              ccl::group_split_type group,
              ccl::device_topology_type mode,
              class kernel_params>
    using gpu_kernel_t =
        typename gpu_module_t<algo_type, group, mode>::template get_kernel<kernel_params>;

    static constexpr const char* name_impl() {
        return "SOURCE_IPC_GPU";
    }

    ccl_ipc_source_gpu_comm(ccl_device& assigned_device,
                            typename base::comm_rank_t idx,
                            device_t& process_device,
                            ccl::group_split_type group_id,
                            ccl::device_topology_type class_id)
            : base(assigned_device, idx),
              inprocess_gpu_comm(process_device) {
        //register in topology
        switch (group_id) {
            case ccl::group_split_type::cluster: {
                switch (class_id) {
                    case ccl::device_topology_type::ring: {
                        const auto& original_rank =
                            inprocess_gpu_comm
                                .template get_comm_data<ccl::group_split_type::cluster,
                                                        ccl::device_topology_type::ring>();
                        base::template reset_rank<ccl::group_split_type::cluster,
                                                  ccl::device_topology_type::ring>(
                            original_rank.rank, original_rank.size);
                        break;
                    }
                    case ccl::device_topology_type::a2a: {
                        const auto& original_rank =
                            inprocess_gpu_comm
                                .template get_comm_data<ccl::group_split_type::cluster,
                                                        ccl::device_topology_type::a2a>();
                        base::template reset_rank<ccl::group_split_type::cluster,
                                                  ccl::device_topology_type::a2a>(
                            original_rank.rank, original_rank.size);
                        break;
                    }
                    default: {
                        throw std::runtime_error(
                            std::string("ccl_ipc_source_gpu_comm must be created") +
                            " unknown topology class: " + std::to_string(class_id));
                    }
                }
                break;
            }
            default: {
                throw std::runtime_error(
                    std::string("ccl_ipc_source_gpu_comm must be created") +
                    "for process-based topology, but requested: " +
                    std::to_string(
                        static_cast<typename std::underlying_type<ccl::group_split_type>::type>(
                            group_id)));
            }
        }
    }

    ~ccl_ipc_source_gpu_comm() = default;

    //TODO L0 work
    device_t& get_impl() {
        return inprocess_gpu_comm;
    }

    std::string to_string_impl() const {
        std::string ret(name_impl());
        ret = ret + "(" + inprocess_gpu_comm.to_string_impl() + ")";
        return ret;
    }
    /*
    template<ccl::group_split_type group_id>
    topology_addr<group_id> get_comm_data() const
    {
        return inprocess_gpu_comm.template get_comm_data<group_id>();
    }
*/
    template <ccl_coll_type module_type,
              ccl::group_split_type group_id,
              ccl::device_topology_type class_id,
              class kernel_params>
    gpu_kernel_t<module_type, group_id, class_id, kernel_params>& get_gpu_kernel() {
        return inprocess_gpu_comm
            .template get_gpu_kernel<module_type, group_id, class_id, kernel_params>();
    }

    template <class kernel_params,
              ccl::group_split_type group_id,
              ccl::device_topology_type class_id,
              class gpu_entry>
    gpu_kernel_t<gpu_entry::type(), group_id, class_id, kernel_params>& register_entry(
        gpu_entry& entry) {
        static_assert(group_id == ccl::group_split_type::cluster,
                      "ccl_ipc_source_gpu_comm available for ccl::group_split_type::cluster only");
        const topology_addr<group_id, class_id>& comm_addr =
            base::template get_comm_data<group_id, class_id>();
        LOG_DEBUG("entry: ", gpu_entry::class_name(), " registered on: ", comm_addr.to_string());

        auto& main_func = get_gpu_kernel<gpu_entry::type(), group_id, class_id, kernel_params>();
        main_func.set_rank(comm_addr.rank);
        main_func.set_size(comm_addr.size);

        ipc_invoke_params<gpu_entry::type(), kernel_params> params(entry.get_ipc_data());
        this->template invoke<group_id, class_id>(entry.get_ipc_session_key(), std::move(params));

        return main_func;
    }

private:
    device_t& inprocess_gpu_comm;
};
} // namespace native
