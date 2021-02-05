#pragma once

#include "common_platform_fixture.hpp"

#include "stubs/stub_platform.hpp"
#include "stubs/stub_context.hpp"

#include <map>

#include "oneapi/ccl/config.h"
#define private public
#define protected public
#include "common/comm/l0/topology/ring_topology.hpp"
#include "common/comm/l0/device_community.hpp"
#include "common/comm/l0/context/device_group_ctx.hpp"
#include "common/comm/l0/context/thread_group_ctx.hpp"
#include "common/comm/l0/context/process_group_ctx.hpp"
#include "common/comm/l0/topology/ring_topology.hpp"

#include "common/comm/l0/modules/modules_source_data.hpp"
#include "common/comm/l0/context/device_storage.hpp"
#include "common/comm/host_communicator/host_communicator.hpp"
#undef protected
#undef private

class router_fixture : public platform_fixture {
public:
    using tracer::set_error;

    router_fixture()
            : platform_fixture(get_global_device_indices() /*"[0:0:0],[0:1:0],[0:2:0],[0:3:0]"*/) {}

    virtual ~router_fixture() {}

    static constexpr const char* ut_cluster_name() {
        return "unit_tests";
    }

    virtual void SetUp() override {
        create_global_platform();
        create_local_platform();

        const auto& modules =
            native::modules_src_container<ccl_coll_allgatherv, ccl_coll_allreduce>::instance()
                .get_modules_collection<ccl_coll_allreduce>();
        if (modules.end() == modules.find(ccl::device_topology_type::ring)) {
            native::modules_src_container<ccl_coll_allgatherv, ccl_coll_allreduce>::instance()
                .load_kernel_source<ccl_coll_allreduce>("kernels/ring_allreduce.spv",
                                                        ccl::device_topology_type::ring);
        }
    }

    virtual void TearDown() override {}

    void init_routing_data(size_t proc_idx) {
        process_idx = proc_idx;

        pg_comm.reset(new stub::process_context(std::shared_ptr<ccl::host_communicator>(
            new ccl::host_communicator))); //TODO use rank & size
        tg_comm = pg_comm->thread_group_ctx.get();
    }

    void fill_indices_data(size_t thread_idx, ccl::device_indices_type data) {
        idx[thread_idx] = data;
        stub::make_stub_devices(data);
    }

    void create_devices_by_affinity(
        size_t thread_idx,
        std::initializer_list<typename ccl::device_indices_type::value_type> data) {
        create_devices_by_affinity(thread_idx, ccl::device_indices_type(data));
    }

    void create_devices_by_affinity(size_t thread_idx, ccl::device_indices_type data) {
        using namespace native;

        fill_indices_data(thread_idx, data);

        ccl::context_comm_addr comm_addr;
        comm_addr.thread_idx = thread_idx;
        comm_addr.thread_count = tg_comm->per_thread_indices.size();

        thread_group_context::device_group_ctx_ptr group_ctx(
            new device_group_context(comm_addr, idx[thread_idx]));
        /*
                                device_group_context::create(comm_addr, idx[thread_idx],
                                                             *pg_comm->gpu_device_storage);
*/
        tg_comm->thread_device_group_ctx[thread_idx] = group_ctx;

        // prepare device communities
        {
            auto& ring_container = tg_comm->thread_device_topology[thread_idx]
                                       .get_community<ccl::device_topology_type::ring>();
            (void)ring_container;

            auto& a2a_container = tg_comm->thread_device_topology[thread_idx]
                                      .get_community<ccl::device_topology_type::a2a>();
            a2a_container.set_topology(
                std::make_shared<device_community<ccl::device_topology_type::a2a>>(comm_addr));
        }
        {
            auto& ring_container = pg_comm->process_device_topology[comm_addr.thread_idx]
                                       .get_community<ccl::device_topology_type::ring>();
            (void)ring_container;

            auto& a2a_container = pg_comm->process_device_topology[comm_addr.thread_idx]
                                      .get_community<ccl::device_topology_type::a2a>();
            a2a_container.set_topology(
                std::make_shared<device_community<ccl::device_topology_type::a2a>>(comm_addr));
        }
    }

    const ccl::device_indices_type& get_device_affinity(size_t thread_id) const {
        auto it = idx.find(thread_id);
        if (it == idx.end()) {
            set_error(__PRETTY_FUNCTION__);
            std::cerr << "invalid thread id: " << thread_id << std::endl;
            dump();
            abort();
        }
        return it->second;
    }

    size_t process_idx = 0;

    native::thread_group_context* tg_comm;
    std::unique_ptr<stub::process_context> pg_comm;

    native::thread_group_context* get_thread_group_ctx() {
        return tg_comm;
    }

    struct process_creator_params {
        ccl::process_device_indices_type total_node_mask;
        size_t process_index;
        size_t cluster_device_rank_offset;
        size_t cluster_device_size;
        std::vector<size_t> thread_ids;
        std::vector<size_t> process_ids;
    };

    process_creator_params prepare_process_params(
        size_t process_index,
        std::initializer_list<typename ccl::process_device_indices_type::value_type> thread_indices,
        std::initializer_list<typename ccl::process_device_indices_type::value_type>
            total_cluster_indices,
        const std::map<size_t, ccl::host_id>& process_to_cluster_table =
            std::map<size_t, std::string>{}) {
        init_routing_data(process_index);
        process_creator_params ret;
        ret.process_index = process_index;

        //prepare thread context
        tg_comm->per_thread_indices = thread_indices;
        for (const auto& thread_id : tg_comm->per_thread_indices) {
            create_devices_by_affinity(thread_id.first, thread_id.second);
            ret.thread_ids.push_back(thread_id.first);
        }

        ret.total_node_mask = total_cluster_indices;
        for (const auto& prc : ret.total_node_mask) {
            ret.process_ids.push_back(prc.first);

            //find cluster name for process id
            ccl::host_id node_name = ut_cluster_name();
            auto name_candidate_it = process_to_cluster_table.find(prc.first);
            if (name_candidate_it != process_to_cluster_table.end()) {
                node_name = name_candidate_it->second;
            }

            pg_comm->set_node_afinity_indices(node_name, prc.first, prc.second);
            stub::make_stub_devices(prc.second);
        }

        ret.cluster_device_rank_offset = 0;
        ret.cluster_device_size = 0;
        for (const auto& cluster_value : ret.total_node_mask) {
            if (cluster_value.first < process_idx) {
                ret.cluster_device_rank_offset += cluster_value.second.size();
            }
            ret.cluster_device_size += cluster_value.second.size();
        }
        return ret;
    }

private:
    std::map<size_t, ccl::device_indices_type> idx;
};
