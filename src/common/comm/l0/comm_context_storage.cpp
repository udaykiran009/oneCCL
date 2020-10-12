#include "common/comm/host_communicator/host_communicator.hpp"

#include "common/comm/comm.hpp"
#include "common/comm/l0/comm_context.hpp"
#include "common/comm/l0/comm_context_storage.hpp"

#include "common/global/global.hpp"

namespace ccl {
group_context& group_context::instance() {
    static group_context inst;
    return inst;
}

group_context::comm_group_t group_context::group_by_kvs(
    const std::vector<size_t>& local_thread_device_group_ranks,
    size_t cluster_device_group_size,
    std::shared_ptr<kvs_interface> kvs) {

    LOG_INFO("Thread acquire by barrier");
    std::shared_ptr<ikvs_wrapper> kvs_wrap = std::shared_ptr<ikvs_wrapper>(new users_kvs(kvs));
    std::shared_ptr<atl_wrapper> atl =
        std::shared_ptr<atl_wrapper>(new atl_wrapper(cluster_device_group_size,
                                                    local_thread_device_group_ranks,
                                                    kvs_wrap));
    LOG_INFO("Thread released by barrier");
    std::cout <<"Check cluster_device_group_size: " << cluster_device_group_size << \
              " "<< local_thread_device_group_ranks.size() <<std::endl;
    for (size_t i = 0; i < local_thread_device_group_ranks.size(); i++){
         std::cout <<"Check local_thread_device_group_ranks: " << local_thread_device_group_ranks[i] << std::endl;
    }
    // register group slot in global context table, based on communicator id
    comm_group_t group = group_context::group_by_comm(atl);

    // sync existing group: blocking operation - wait for all groups
    LOG_INFO("group thread barrier acquired: ", static_cast<void*>(group.get()));
    group->sync_group_size(local_thread_device_group_ranks.size());
    LOG_INFO("group thread barrier released: ", static_cast<void*>(group.get()));
    return group;
}

group_context::comm_group_t group_context::group_by_comm(std::shared_ptr<atl_wrapper> atl) {

    LOG_INFO("\n",
         "\nATL info:",
         "\n  threads count:                    ",
         atl->get_threads_count(),
         "\n  devices per rank count:           ",
         atl->get_devices_per_rank_count(),
         "\n  atl size:                         ",
         atl->get_size(),
         "\n  rank:                             ",
         atl->get_rank(),
        "\n  unique id of atl comm:             ",
         atl->get_id(),
         "\n")

    comm_group_t group;
    {
        // mutex
        std::unique_lock<ccl_spinlock> lock(mutex);
        size_t threads_count = atl->get_threads_count();
        size_t on_process_ranks_count = atl->get_devices_per_rank_count();
        group_context::group_unique_key unique_id = atl->get_id();

        auto ctx_it = communicator_group_map.find(unique_id);
        if (ctx_it == communicator_group_map.end()) {
            std::shared_ptr<host_communicator> host_comm =
                        std::make_shared<host_communicator>(atl);
            group.reset(
                new ccl::comm_group(host_comm, threads_count, on_process_ranks_count, unique_id));
            communicator_group_map.insert({ unique_id, group });
            LOG_INFO("Comm group: ",
                     static_cast<void*>(group.get()),
                     " has been created for unique_id: ",
                     unique_id,
                     ", expected thread count: ",
                     threads_count,
                     ", on process rank count: ",
                     on_process_ranks_count);
        }
        else {
            group = ctx_it->second;
            LOG_INFO("get existing comm group: ",
                     static_cast<void*>(group.get()),
                     " for unique_id: ",
                     unique_id);
        }
    }
    return group;
}

group_context::comm_group_t group_context::get_existing_group_by_id(
    const group_unique_key& unique_id) {
    comm_group_t group;
    LOG_DEBUG("get existing comm group by id: ",
              unique_id,
              ", total groups: ",
              communicator_group_map.size());
    {
        std::unique_lock<ccl_spinlock> lock(mutex);
        auto ctx_it = communicator_group_map.find(unique_id);
        if (ctx_it == communicator_group_map.end()) {
            std::stringstream ss;
            ss << "Cannot find `comm_group_t` by id: " << unique_id << std::endl;
            const std::string mess = ss.str();
            LOG_ERROR(mess);
            throw ccl::exception(std::string(__FUNCTION__) + " - " + mess);
        }
        else {
            group = ctx_it->second;
            LOG_DEBUG("get existing comm group: ",
                      static_cast<void*>(group.get()),
                      " for unique_id: ",
                      unique_id);
        }
    }
    return group;
}
} // namespace ccl
