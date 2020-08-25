#if 1
    #include "../tests/unit/api/stubs/host_communicator.hpp"
#else
    #include "common/comm/host_communicator/host_communicator.hpp"
#endif

#include "common/comm/comm.hpp"
#include "common/comm/l0/comm_context.hpp"
#include "common/comm/l0/comm_context_storage.hpp"

#include "common/global/global.hpp"

namespace ccl
{
group_context& group_context::instance()
{
    static group_context inst;
    return inst;
}

group_context::comm_group_t group_context::group_by_kvs(const std::vector<size_t>& local_thread_device_group_ranks,
                                                        size_t cluster_device_group_size,
                                                        std::shared_ptr<kvs_interface> kvs)
{
    //TODO
    static ccl_comm_id_storage::comm_id TODO_TMP_ID = ccl::global_data::get().comm_ids->acquire();

    //barrier operation acquire: wait while all threads from all processes enters here...
    std::shared_ptr<host_communicator> host_comm =
            std::make_shared<host_communicator>(std::make_shared<ccl_comm>(local_thread_device_group_ranks, cluster_device_group_size, kvs,
            TODO_TMP_ID.clone()));
    //barrier operation release: every threads continue its execution here...

    // register group slot in global context table, based on communicator id
    comm_group_t group = group_context::group_by_comm(host_comm);

    // sync existing group: blocking operation - wait for all groups
    group->sync_group_size(local_thread_device_group_ranks.size());
    return group;
}

group_context::comm_group_t group_context::group_by_comm(shared_communicator_t host_comm)
{
    group_context::group_unique_key unique_id = host_comm->comm_impl->id();
    size_t threads_count = host_comm->comm_impl->thread_count();
    size_t on_process_ranks_count = host_comm->comm_impl->on_process_ranks_count();

    comm_group_t group;
    {
        std::unique_lock<ccl_spinlock> lock(mutex);
        auto ctx_it = communicator_group_map.find(unique_id);
        if(ctx_it == communicator_group_map.end())
        {
            group.reset(new ccl::comm_group(host_comm,
                                            threads_count,
                                            on_process_ranks_count));
            communicator_group_map.insert({
                                                        unique_id,
                                                        group
                                                        });
        }
        else
        {
            group = ctx_it->second;
        }
    }
    return group;
}
}
