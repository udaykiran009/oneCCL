#include "oneapi/ccl/ccl_exception.hpp"
#include "common/comm/comm_interface.hpp"
#include "common/comm/single_device_communicator/single_device_communicator.hpp"

namespace ccl {

/**
 * Base dispatcher for conversion vector to map
 */

template <class Context>
struct context_extractor
{
    static Context& extract(Context &ctx) {return ctx; }
};

template<>
struct context_extractor<ccl::context>
{
    static typename unified_context_type::ccl_native_t& extract(ccl::context &ctx) {return ctx.get_native(); }
};

template<class impl>
struct comm_impl_base_dispatch
{
    static void validate_contract(const size_t cluster_devices_size,
                                  const size_t table_size)
    {
        if (table_size == 0)
        {
            throw ccl::invalid_argument("API", "create_communicators", "`local_rank_device_map` cannot be empty");
        }

        if (table_size > cluster_devices_size)
        {
            throw ccl::invalid_argument("API", "create_communicators", "`local_rank_device_map` size: " +
                                    std::to_string(table_size) + " must not exceed total size: " +
                                    std::to_string(cluster_devices_size));
        }
    }

    template <class DeviceType, class ContextType>
    static vector_class<communicator> create_communicators_selector(
                const size_t cluster_devices_size, /*global devices count*/
                const vector_class<pair_class<int, DeviceType>>& local_rank_device_map,
                ContextType& context,
                shared_ptr_class<kvs_interface> kvs) {

        map_class<int, DeviceType> converted_map;
        std::transform(local_rank_device_map.begin(), local_rank_device_map.end(),
                       std::inserter(converted_map, converted_map.end()), [](const pair_class<int, DeviceType>& val)
                       {
                           return std::make_pair(val.first, val.second);
                       });
        if (local_rank_device_map.size() != converted_map.size())
        {
            std::stringstream ss;
            ss << "found duplicated ranks in `local_rank_device_map`:\n";
            for (const auto& v : local_rank_device_map)
            {
                ss << std::to_string(v.first) << ", ";
            }
            throw ccl::invalid_argument("API", "create_communicators", ss.str());
        }
        return impl::template create_communicators_selector(cluster_devices_size, converted_map, context_extractor<ContextType>::extract(context), kvs);
    }
};

template<cl_backend_type type>
struct comm_impl_dispatch_selector {
};

#if !defined(CCL_ENABLE_SYCL) and !defined(MULTI_GPU_SUPPORT)
template<>
struct comm_impl_dispatch_selector<cl_backend_type::empty_backend> :
 public comm_impl_base_dispatch<comm_impl_dispatch_selector<cl_backend_type::empty_backend>>
{
    using base_t = comm_impl_base_dispatch<comm_impl_dispatch_selector<cl_backend_type::empty_backend>>;

    template <class DeviceType, class ContextType>
    static vector_class<communicator> create_communicators_selector(
                const size_t cluster_devices_size, /*global devices count*/
                const vector_class<pair_class<int, DeviceType>>& local_rank_device_map,
                ContextType& context,
                shared_ptr_class<kvs_interface> kvs)
    {
        return base_t::template create_communicators_selector<DeviceType>(cluster_devices_size, local_rank_device_map, context_extractor<ContextType>::extract(context), kvs);
    }

    template <class DeviceType, class ContextType>
    static vector_class<communicator> create_communicators_selector(
                const size_t cluster_devices_size, /*global devices count*/
                const map_class<int, DeviceType>& local_rank_device_map,
                ContextType& context,
                shared_ptr_class<kvs_interface> kvs) {

        base_t::validate_contract(cluster_devices_size, local_rank_device_map.size());
        if (local_rank_device_map.size() != 1)
        {
            throw ccl::unsupported("API", "create_communicators", "`local_rank_device_map` size: " +
                                   std::to_string(local_rank_device_map.size()) + " but must be: 1 for your configuration: " +
                                   backend_traits::name());
        }

        LOG_TRACE("Create host communicator");

        ccl::communicator_interface_ptr impl =
            ccl::communicator_interface::create_communicator_impl(cluster_devices_size, local_rank_device_map.begin()->first, kvs);
        ccl::vector_class<ccl::communicator> ret;
        ret.push_back(ccl::communicator(std::move(impl)) );
        return ret;
    }
};
#endif

#if defined(CCL_ENABLE_SYCL) and !defined(MULTI_GPU_SUPPORT)
template<>
struct comm_impl_dispatch_selector<cl_backend_type::dpcpp_sycl> :
 public comm_impl_base_dispatch<comm_impl_dispatch_selector<cl_backend_type::dpcpp_sycl>>
{
    using base_t = comm_impl_base_dispatch<comm_impl_dispatch_selector<cl_backend_type::dpcpp_sycl>>;

    template <class DeviceType, class ContextType>
    static vector_class<communicator> create_communicators_selector(
                const size_t cluster_devices_size, /*global devices count*/
                const vector_class<pair_class<int, DeviceType>>& local_rank_device_map,
                ContextType& context,
                shared_ptr_class<kvs_interface> kvs)
    {
        return base_t::template create_communicators_selector<DeviceType>(cluster_devices_size, local_rank_device_map, context_extractor<ContextType>::extract(context), kvs);
    }

    template <class ContextType>
    static vector_class<communicator> create_communicators_selector(
                const size_t cluster_devices_size, /*global devices count*/
                const map_class<int, ccl::device_index_type>& local_rank_device_map,
                ContextType& context,
                shared_ptr_class<kvs_interface> kvs) {

        base_t::validate_contract(cluster_devices_size, local_rank_device_map.size());

        map_class<int, cl::sycl::device> converted_device_map;
        std::transform(local_rank_device_map.begin(), local_rank_device_map.end(),
                       std::inserter(converted_device_map, converted_device_map.end()),
                       [](const typename map_class<int, ccl::device_index_type>::value_type& val)
                       {
                           return std::make_pair(val.first, ccl::create_from_index(val.second).get());
                       });
        return create_communicators_selector(cluster_devices_size, converted_device_map, context_extractor<ContextType>::extract(context), kvs);
    }

    template <class ContextType>
    static vector_class<communicator> create_communicators_selector(
                const size_t cluster_devices_size, /*global devices count*/
                const map_class<int,ccl::device>& local_rank_device_map,
                ContextType& context,
                shared_ptr_class<kvs_interface> kvs) {

        map_class<int, cl::sycl::device> converted_device_map;
        std::transform(local_rank_device_map.begin(), local_rank_device_map.end(),
                       std::inserter(converted_device_map, converted_device_map.end()),
                       [](const typename map_class<int,ccl::device>::value_type& val)
                       {
                           return std::make_pair(val.first, val.second.get_native());
                       });
        return create_communicators_selector(cluster_devices_size, converted_device_map, context_extractor<ContextType>::extract(context), kvs);
    }

    template <class ContextType>
    static vector_class<communicator> create_communicators_selector(
                const size_t cluster_devices_size, /*global devices count*/
                const map_class<int, cl::sycl::device>& local_rank_device_map,
                ContextType& context,
                shared_ptr_class<kvs_interface> kvs) {

        base_t::validate_contract(cluster_devices_size, local_rank_device_map.size());

        auto it = local_rank_device_map.begin();
        const cl::sycl::device& dev = it->second;

        if (dev.is_host())
        {
            it = std::find_if_not(local_rank_device_map.begin(), local_rank_device_map.end(),
                                  [](const typename map_class<int, cl::sycl::device>::value_type& val) {
                                    return val.second.is_host();
                                });
        }
        else if(dev.is_cpu())
        {
            it = std::find_if_not(local_rank_device_map.begin(), local_rank_device_map.end(),
                                  [](const typename map_class<int, cl::sycl::device>::value_type& val) {
                                    return val.second.is_cpu();
                                });
        }
        else if(dev.is_gpu())
        {
            it = std::find_if_not(local_rank_device_map.begin(), local_rank_device_map.end(),
                                  [](const typename map_class<int, cl::sycl::device>::value_type& val) {
                                    return val.second.is_gpu();
                                });
        }
        else if(dev.is_accelerator())
        {
            it = std::find_if_not(local_rank_device_map.begin(), local_rank_device_map.end(),
                                  [](const typename map_class<int, cl::sycl::device>::value_type& val) {
                                    return val.second.is_accelerator();
                                });
        }
        else
        {
            throw ccl::invalid_argument("API", "create_communicators", "invalid `cl::sycl::device` type");
        }

        if (it != local_rank_device_map.end())
        {
            throw ccl::invalid_argument("API", "create_communicators", "mixed collection of `cl::sycl::device` are not supported");
        }


        if (local_rank_device_map.size() != 1)
        {
            throw ccl::unsupported("API", "create_communicators", "`local_rank_device_map` size: " +
                                   std::to_string(local_rank_device_map.size()) + " but must be: 1 for your configuration: " +
                                   backend_traits::name());
        }

        // if (dev.is_host() || dev.is_cpu())
        // {
        //     LOG_DEBUG("Create host communicator");

        //     ccl::communicator_interface_ptr impl =
        //         ccl::communicator_interface::create_communicator_impl(cluster_devices_size, local_rank_device_map.begin()->first, kvs);

        //     ccl::vector_class<ccl::communicator> ret;
        //     ret.push_back(ccl::communicator(std::move(impl)));
        //     return ret;
        // }

        /* reset iterator after std::find_if_not */
        it = local_rank_device_map.begin();
        int rank = it->first;
        auto& device = it->second;

        LOG_DEBUG("Create single device communicator from SYCL device (sycl and !mgpu), after find_if rank ", rank);

        std::shared_ptr<ikvs_wrapper> kvs_wrapper(new users_kvs(kvs));
        std::shared_ptr<atl_wrapper> atl =
        std::shared_ptr<atl_wrapper>(new atl_wrapper(cluster_devices_size, { rank }, kvs_wrapper));

        ccl::comm_split_attr attr = create_comm_split_attr(
        ccl::attr_val<ccl::comm_split_attr_id::group>(ccl::group_split_type::undetermined));
        ccl::communicator_interface_ptr impl =
        ccl::communicator_interface::create_communicator_impl(device, context, rank, cluster_devices_size, attr, atl);

        //TODO use gpu_comm_attr to automatically visit()
        //auto single_dev_comm = std::dynamic_pointer_cast<single_device_communicator>(impl);
        //single_dev_comm->set_context(context);
        ccl::vector_class<ccl::communicator> ret;
        ret.push_back(ccl::communicator(std::move(impl)));
        return ret;

    /*
    //collect ranks
    vector_class<int> local_thread_ranks;
    local_thread_ranks.reserve(local_rank_device_map.size());
    std::transform(
        local_rank_device_map.begin(),
        local_rank_device_map.end(),
        std::back_inserter(local_thread_ranks),
        [](const typename vector_class<pair_class<int, DeviceType>>::value_type& val) {
            return val.first;
        });

    vector_class<DeviceType> local_thread_devices;
    local_thread_devices.reserve(local_rank_device_map.size());
    std::transform(
        local_rank_device_map.begin(),
        local_rank_device_map.end(),
        std::back_inserter(local_thread_devices),
        [](const typename vector_class<pair_class<int, DeviceType>>::value_type& val) {
            return val.second;
        });

    if ()
    auto ret = thread_group->create_communicators_group(local_thread_devices);
    return ret;
    return {};
    */
    }
};
#endif //defined(CCL_ENABLE_SYCL) and !defined(MULTI_GPU_SUPPORT)

#if defined(MULTI_GPU_SUPPORT) and !defined(CCL_ENABLE_SYCL)
template<>
struct comm_impl_dispatch_selector<cl_backend_type::l0> :
 public comm_impl_base_dispatch<comm_impl_dispatch_selector<cl_backend_type::l0>>
{
    using base_t = comm_impl_base_dispatch<comm_impl_dispatch_selector<cl_backend_type::l0>>;

    template <class DeviceType, class ContextType>
    static vector_class<communicator> create_communicators_selector(
                const size_t cluster_devices_size, /*global devices count*/
                const vector_class<pair_class<int, DeviceType>>& local_rank_device_map,
                ContextType& context,
                shared_ptr_class<kvs_interface> kvs)
    {
        return base_t::template create_communicators_selector<DeviceType>(cluster_devices_size, local_rank_device_map, context_extractor<ContextType>::extract(context), kvs);
    }

    template <class ContextType>
    static vector_class<communicator> create_communicators_selector(
                const size_t cluster_devices_size, /*global devices count*/
                const map_class<int, ccl::device>& local_rank_device_map,
                ContextType& context,
                shared_ptr_class<kvs_interface> kvs) {

        map_class<int, ccl::device_index_type> converted_device_map;
        std::transform(local_rank_device_map.begin(), local_rank_device_map.end(),
                       std::inserter(converted_device_map, converted_device_map.end()),
                       [](const typename map_class<int,ccl::device>::value_type& val)
                       {
                           return std::make_pair(val.first, val.second.get_native()->get_device_path());
                       });
        return create_communicators_selector(cluster_devices_size, converted_device_map, context_extractor<ContextType>::extract(context), kvs);
    }

    template <class ContextType>
    static vector_class<communicator> create_communicators_selector(
                const size_t cluster_devices_size, /*global devices count*/
                const map_class<int, typename unified_device_type::ccl_native_t>& local_rank_device_map,
                ContextType& context,
                shared_ptr_class<kvs_interface> kvs) {

        map_class<int, ccl::device_index_type> converted_device_map;
        std::transform(local_rank_device_map.begin(), local_rank_device_map.end(),
                       std::inserter(converted_device_map, converted_device_map.end()),
                       [](const typename map_class<int, typename unified_device_type::ccl_native_t>::value_type& val)
                       {
                           return std::make_pair(val.first, val.second->get_device_path());
                       });
        return create_communicators_selector(cluster_devices_size, converted_device_map, context_extractor<ContextType>::extract(context), kvs);
    }

    template <class ContextType>
    static vector_class<communicator> create_communicators_selector(
                const size_t cluster_devices_size, /*global devices count*/
                const map_class<int, ccl::device_index_type>& local_rank_device_map,
                ContextType& context,
                shared_ptr_class<kvs_interface> kvs) {

        base_t::validate_contract(cluster_devices_size, local_rank_device_map.size());

        //collect ranks
        vector_class<int> local_thread_ranks;
        local_thread_ranks.reserve(local_rank_device_map.size());
        std::transform(local_rank_device_map.begin(),
                   local_rank_device_map.end(),
                   std::back_inserter(local_thread_ranks),
                   [](const typename map_class<int, ccl::device_index_type>::value_type& val) {
                       return val.first;
                   });
        group_context::comm_group_t thread_group =
            group_context::instance().group_by_kvs(local_thread_ranks, cluster_devices_size, kvs);

        vector_class<ccl::device_index_type> local_thread_devices;
        local_thread_devices.reserve(local_rank_device_map.size());
        std::transform(local_rank_device_map.begin(),
                   local_rank_device_map.end(),
                   std::back_inserter(local_thread_devices),
                   [](const typename map_class<int, ccl::device_index_type>::value_type& val) {
                       return val.second;
                   });

        auto ret = thread_group->create_communicators_group(local_thread_devices, context_extractor<ContextType>::extract(context));
        return ret;
    }
};
#endif //defined(MULTI_GPU_SUPPORT) and !defined(CCL_ENABLE_SYCL)


#if defined(MULTI_GPU_SUPPORT) and defined(CCL_ENABLE_SYCL)
template<>
struct comm_impl_dispatch_selector<cl_backend_type::dpcpp_sycl_l0> :
 public comm_impl_base_dispatch<comm_impl_dispatch_selector<cl_backend_type::dpcpp_sycl_l0>>
{
    using base_t = comm_impl_base_dispatch<comm_impl_dispatch_selector<cl_backend_type::dpcpp_sycl_l0>>;

    template <class DeviceType, class ContextType>
    static vector_class<communicator> create_communicators_selector(
                const size_t cluster_devices_size, /*global devices count*/
                const vector_class<pair_class<int, DeviceType>>& local_rank_device_map,
                ContextType& context,
                shared_ptr_class<kvs_interface> kvs)
    {
        return base_t::template create_communicators_selector<DeviceType>(cluster_devices_size, local_rank_device_map, context_extractor<ContextType>::extract(context), kvs);
    }

    template <class ContextType>
    static vector_class<communicator> create_communicators_selector(
                const size_t cluster_devices_size, /*global devices count*/
                const map_class<int, ccl::device>& local_rank_device_map,
                ContextType& context,
                shared_ptr_class<kvs_interface> kvs) {

        map_class<int, cl::sycl::device> converted_device_map;
        std::transform(local_rank_device_map.begin(), local_rank_device_map.end(),
                       std::inserter(converted_device_map, converted_device_map.end()),
                       [](const typename map_class<int, ccl::device>::value_type& val)
                       {
                           return std::make_pair(val.first, val.second.get_native());
                       });
        return create_communicators_selector(cluster_devices_size, converted_device_map, context_extractor<ContextType>::extract(context), kvs);
    }

    // TODO: try to combine these 2 overload below
    template <class ContextType>
    static vector_class<communicator> create_communicators_selector(
                const size_t cluster_devices_size, /*global devices count*/
                const map_class<int, cl::sycl::device>& local_rank_device_map,
                ContextType& context,
                shared_ptr_class<kvs_interface> kvs) {

        base_t::validate_contract(cluster_devices_size, local_rank_device_map.size());

        //collect ranks
        vector_class<int> local_thread_ranks;
        local_thread_ranks.reserve(local_rank_device_map.size());
        std::transform(local_rank_device_map.begin(),
                   local_rank_device_map.end(),
                   std::back_inserter(local_thread_ranks),
                   [](const typename map_class<int, cl::sycl::device>::value_type& val) {
                       return val.first;
                   });
        group_context::comm_group_t thread_group =
            group_context::instance().group_by_kvs(local_thread_ranks, cluster_devices_size, kvs);

        vector_class<cl::sycl::device> local_thread_devices;
        local_thread_devices.reserve(local_rank_device_map.size());
        std::transform(local_rank_device_map.begin(),
                   local_rank_device_map.end(),
                   std::back_inserter(local_thread_devices),
                   [](const typename map_class<int, cl::sycl::device>::value_type& val) {
                       return val.second;
                   });

        auto ret = thread_group->create_communicators_group(local_thread_devices, context_extractor<ContextType>::extract(context));
        return ret;
    }

    template <class ContextType>
    static vector_class<communicator> create_communicators_selector(
                const size_t cluster_devices_size, /*global devices count*/
                const map_class<int, ccl::device_index_type>& local_rank_device_map,
                ContextType& context,
                shared_ptr_class<kvs_interface> kvs) {

        base_t::validate_contract(cluster_devices_size, local_rank_device_map.size());

        //collect ranks
        vector_class<int> local_thread_ranks;
        local_thread_ranks.reserve(local_rank_device_map.size());
        std::transform(local_rank_device_map.begin(),
                   local_rank_device_map.end(),
                   std::back_inserter(local_thread_ranks),
                   [](const typename map_class<int, ccl::device_index_type>::value_type& val) {
                       return val.first;
                   });
        group_context::comm_group_t thread_group =
            group_context::instance().group_by_kvs(local_thread_ranks, cluster_devices_size, kvs);

        vector_class<ccl::device_index_type> local_thread_devices;
        local_thread_devices.reserve(local_rank_device_map.size());
        std::transform(local_rank_device_map.begin(),
                   local_rank_device_map.end(),
                   std::back_inserter(local_thread_devices),
                   [](const typename map_class<int, ccl::device_index_type>::value_type& val) {
                       return val.second;
                   });

        auto ret = thread_group->create_communicators_group(local_thread_devices, context_extractor<ContextType>::extract(context));
        return ret;
    }
};
#endif //defined(MULTI_GPU_SUPPORT) and defined(CCL_ENABLE_SYCL)
}
