#pragma once
#include "../fixture.hpp"


#define private public
#define protected public
#include <map>

#include "ccl_config.h"
#include "common/comm/l0/topology/ring_topology.hpp"
#include "common/comm/l0/device_community.hpp"
#include "common/comm/l0/context/device_group_ctx.hpp"
#include "common/comm/l0/context/thread_group_ctx.hpp"
#include "common/comm/l0/context/process_group_ctx.hpp"
#include "common/comm/l0/topology/ring_topology.hpp"

#include "common/comm/l0/modules/modules_source_data.hpp"
#include "common/comm/l0/context/device_storage.hpp"
#undef protected
#undef private


class router_fixture : public common_fixture
{
public:
    using tracer::set_error;

    router_fixture() :
     common_fixture("[0:0:0],[0:1:0],[0:2:0],[0:3:0]")
    {
    }

    virtual ~router_fixture()
    {
    }

    virtual void SetUp() override
    {
        create_global_platform();
        local_affinity = global_affinities.at(0);
        create_local_platform();

        const auto& modules = native::modules_src_container<ccl_coll_allgatherv,ccl_coll_allreduce>::instance().get_modules_collection<ccl_coll_allreduce>();
        if (modules.end() == modules.find(ccl::device_topology_class::ring_class))
        {
            native::modules_src_container<ccl_coll_allgatherv,ccl_coll_allreduce>::instance().load_kernel_source<ccl_coll_allreduce>("kernels/ring_allreduce.spv", ccl::device_topology_class::ring_class);
        }
    }

    virtual void TearDown() override
    {
    }

    void init_routing_data(size_t proc_idx)
    {
        process_idx = proc_idx;

        pg_comm.reset( new native::process_group_context(ccl::environment::instance().create_communicator()));
        tg_comm = pg_comm->thread_group_ctx.get();
    }

    void fill_indices_data(size_t thread_idx, ccl::device_indices_t data)
    {
        idx[thread_idx] = data;
    }

    std::shared_ptr<native::specific_plain_device_storage>
            create_devices_by_affinity(size_t thread_idx,
                                       std::initializer_list< typename ccl::device_indices_t::value_type> data)
    {
        return create_devices_by_affinity(thread_idx,ccl::device_indices_t (data));
    }

    std::shared_ptr<native::specific_plain_device_storage>
            create_devices_by_affinity(size_t thread_idx, ccl::device_indices_t data)
    {
        using namespace native;

        fill_indices_data(thread_idx, data);

        ccl::context_comm_addr comm_addr;
        comm_addr.thread_idx = thread_idx;
        comm_addr.thread_count = tg_comm->per_thread_indices.size();

        thread_group_context::device_group_ctx_ptr group_ctx =
                                device_group_context::create(comm_addr, idx[thread_idx],
                                                             *pg_comm->gpu_device_storage);
        tg_comm->thread_device_group_ctx[thread_idx] = group_ctx;
        tg_comm->thread_device_topology[thread_idx] =
                std::make_tuple(std::make_shared<device_community<ccl::device_topology_type::thread_group_ring>>(comm_addr),
                                std::make_shared<device_community<ccl::device_topology_type::thread_group_torn_apart_ring>>(comm_addr),
                                std::make_shared<device_community<ccl::device_topology_type::a2a_thread_group>>(comm_addr));
        pg_comm->process_device_topology[comm_addr.thread_idx] =
                std::make_tuple(std::make_shared<device_community<ccl::device_topology_type::allied_process_group_ring>>(comm_addr),
                                std::make_shared<device_community<ccl::device_topology_type::process_group_torn_apart_ring>>(comm_addr),
                                std::make_shared<device_community<ccl::device_topology_type::a2a_allied_process_group>>(comm_addr));
        return pg_comm->gpu_device_storage->thread_gpu_comms.find(thread_idx)->second;
    }

    const ccl::device_indices_t& get_device_affinity(size_t thread_id) const
    {
        auto it = idx.find(thread_id);
        if(it == idx.end())
        {
            set_error(__PRETTY_FUNCTION__);
            std::cerr << "invalid thread id: " << thread_id << std::endl;
            dump();
            abort();
        }
        return it->second;
    }

    size_t process_idx = 0;

    native::thread_group_context* tg_comm;
    std::unique_ptr<native::process_group_context> pg_comm;

    struct process_creator_params
    {
        ccl::process_device_indices_t total_node_mask;
        size_t cluster_device_rank_offset;
        size_t cluster_device_size;
        std::vector<size_t> thread_ids;
        std::vector<size_t> process_ids;
    };


    process_creator_params prepare_process_params(
            size_t process_index,
            std::initializer_list<typename ccl::process_device_indices_t::value_type> thread_indices,
            std::initializer_list<typename ccl::process_device_indices_t::value_type> total_cluster_indices)
    {

        init_routing_data(process_index);
        process_creator_params ret;

        //prepare thread context
        tg_comm->per_thread_indices = thread_indices;
        for(const auto& thread_id : tg_comm->per_thread_indices)
        {
            create_devices_by_affinity(thread_id.first, thread_id.second);
            ret.thread_ids.push_back(thread_id.first);
        }

        ret.total_node_mask = total_cluster_indices;
        for (const auto &prc : ret.total_node_mask)
        {
            ret.process_ids.push_back(prc.first);
            pg_comm->set_node_afinity_indices("unit_test", prc.first, prc.second);
        }



        ret.cluster_device_rank_offset = 0;
        ret.cluster_device_size = 0;
        for (const auto& cluster_value : ret.total_node_mask)
        {
            if (cluster_value.first < process_idx)
            {
                ret.cluster_device_rank_offset += cluster_value.second.size();
            }
            ret.cluster_device_size += cluster_value.second.size();
        }
        return ret;
    }

private:
    std::map<size_t, ccl::device_indices_t> idx;
};
