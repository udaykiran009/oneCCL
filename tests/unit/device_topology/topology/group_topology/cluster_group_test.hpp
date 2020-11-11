#pragma once
#include <initializer_list>
#include "common/comm/l0/topology/ring/cluster_group_device_creator_impl.hpp"
#include "../topology_utils.hpp"

namespace topology_suite {

static ccl::device_index_type dev_0(0, 0, ccl::unused_index_value);
static ccl::device_index_type dev_1(0, 1, ccl::unused_index_value);
static ccl::device_index_type dev_2(0, 2, ccl::unused_index_value);
static ccl::device_index_type dev_3(0, 3, ccl::unused_index_value);
static ccl::device_index_type dev_4(0, 4, ccl::unused_index_value);
static ccl::device_index_type dev_5(0, 5, ccl::unused_index_value);
static ccl::device_index_type dev_6(0, 6, ccl::unused_index_value);
static ccl::device_index_type dev_7(0, 7, ccl::unused_index_value);

TEST_F(router_fixture, cluster_simple_scaleup_test) {
    using namespace native;
    using namespace native::detail;
    using namespace ::utils;

    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;

    {
        stub::make_stub_devices({ dev_0, dev_1, dev_2 });

        size_t process_index = 1;
        process_creator_params params =
            prepare_process_params(process_index,
                                   // indices per threads in process_index
                                   {
                                       { 0, { dev_0, dev_0, dev_0 } },
                                   },
                                   // indices per processes in cluster
                                   {
                                       { 0, { dev_0, dev_0, dev_0 } },
                                       { process_index, { dev_1, dev_1, dev_1 } },
                                       { 2, { dev_2, dev_2, dev_2 } },
                                   });

        // initial device connectivities matrix
        adjacency_matrix expected_matrix{
            { dev_0, { { dev_0, 1 }, { dev_1, 1 }, { dev_2, 1 } } },
            { dev_1, { { dev_0, 1 }, { dev_1, 1 }, { dev_2, 1 } } },
            { dev_2, { { dev_0, 1 }, { dev_1, 1 }, { dev_2, 1 } } },
        };

        // stub for real device connectivity
        native::detail::p2p_rating_function rating_function =
            std::bind(::utils::test_custom_p2p_ping,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      expected_matrix);

        // stub for cluster device topology
        global_sorted_colored_plain_graphs control_cluster_topology{
            { 0,
              {
                  { { 0, dev_0 }, { 0, dev_0 }, { 0, dev_0 } },
              } },
            { process_index,
              {
                  { { 1, dev_1 }, { 1, dev_1 }, { 1, dev_1 } },
              } },
            { 2,
              {
                  { { 2, dev_2 }, { 2, dev_2 }, { 2, dev_2 } },
              } }
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        // build device group context at first
        for (size_t thread_id : params.thread_ids) {
            const ccl::device_indices_type& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, rating_function);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        // skip thread group context
        // create cluster group context
        cluster_group_device_creator top(params.process_index,
                                         params.process_ids.size(),
                                         *pg_comm,
                                         *pg_comm->gpu_device_storage);

        std::stringstream ss;
        adjacency_matrix matrix = cluster_group_device_creator::build_p2p_capability_matrix(
            ss, params.total_node_mask, rating_function);
        //check matrix
        if (matrix != expected_matrix) {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix << "\nExpected matrix:\n"
                   << expected_matrix << std::endl;
            UT_ASSERT(false, "Matrix should be equal to expected_matrix");
        }
        ss.clear();

        // build test topology
        bool res = false;
        std::string descr;

        try {
            ccl::context_comm_addr comm_addr;
            res =
                top.build_all(ss, comm_addr, tg_comm->per_thread_indices, matrix, rating_function);
        }
        catch (const std::exception& ex) {
            res = false;
            descr += std::string("\nfailed with exception: ") + ex.what();
        }

        if (!res) {
            output << ss.str() << std::endl;
            UT_ASSERT(res, "Cannot build cluster topology: " << descr);
        }

        //Check topology
        output << "\n\n\n" << ss.str() << std::endl;

        std::vector<expected_indices_tuple> expected_seq;
        set_control_indices<ccl_gpu_scaleup_proxy<ccl_gpu_comm>>(
            expected_seq, 0, optional_indices{ true, { 3 }, { 0 } });
        set_control_indices<ccl_virtual_gpu_comm>(
            expected_seq, 0, optional_indices{ true, { 4, 5 }, { 1, 2 } });

        std::tie(res, descr) =
            check_ring_multiple_topologies<topology, class_id, process_group_context>(
                pg_comm->process_device_topology, params.thread_ids, expected_seq);
        if (!res) {
            output << "\nLOG:\n" << ss.str() << std::endl;
            output << descr;
            UT_ASSERT(res, descr);
        }
    }
}

TEST_F(router_fixture, cluster_simple_scaleout_test) {
    using namespace native;
    using namespace native::detail;
    using namespace ::utils;

    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;

    {
        stub::make_stub_devices({ dev_0, dev_1, dev_2 });

        size_t process_index = 1;
        process_creator_params params = prepare_process_params(
            process_index,
            // indices per threads in process_index
            {
                { 0, { dev_0, dev_0, dev_0 } },
            },
            // indices per processes in cluster
            {
                { 0, { dev_0, dev_0, dev_0 } },
                { process_index, { dev_1, dev_1, dev_1 } },
                { 2, { dev_2, dev_2, dev_2 } },
            },

            // place process index on differen node name
            { { 0, "node_0" }, { process_index, "process_node" }, { 2, "node_2" } });

        // initial device connectivities matrix
        adjacency_matrix expected_matrix{
            { dev_0, { { dev_0, 1 }, { dev_1, 1 }, { dev_2, 1 } } },
            { dev_1, { { dev_0, 1 }, { dev_1, 1 }, { dev_2, 1 } } },
            { dev_2, { { dev_0, 1 }, { dev_1, 1 }, { dev_2, 1 } } },
        };

        // stub for real device connectivity
        native::detail::p2p_rating_function rating_function =
            std::bind(::utils::test_custom_p2p_ping,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      expected_matrix);

        // stub for cluster device topology
        global_sorted_colored_plain_graphs control_cluster_topology{
            { 0,
              {
                  { { 0, dev_0 }, { 0, dev_0 }, { 0, dev_0 } },
              } },
            { process_index,
              {
                  { { 1, dev_1 }, { 1, dev_1 }, { 1, dev_1 } },
              } },
            { 2,
              {
                  { { 2, dev_2 }, { 2, dev_2 }, { 2, dev_2 } },
              } }
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        // build device group context at first
        for (size_t thread_id : params.thread_ids) {
            const ccl::device_indices_type& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, rating_function);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        // skip thread group context
        // create cluster group context
        cluster_group_device_creator top(params.process_index,
                                         params.process_ids.size(),
                                         *pg_comm,
                                         *pg_comm->gpu_device_storage);

        std::stringstream ss;
        adjacency_matrix matrix = cluster_group_device_creator::build_p2p_capability_matrix(
            ss, params.total_node_mask, rating_function);
        //check matrix
        if (matrix != expected_matrix) {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix << "\nExpected matrix:\n"
                   << expected_matrix << std::endl;
            UT_ASSERT(false, "Matrix should be equal to expected_matrix");
        }
        ss.clear();

        // build test topology
        bool res = false;
        std::string descr;

        try {
            ccl::context_comm_addr comm_addr;
            res =
                top.build_all(ss, comm_addr, tg_comm->per_thread_indices, matrix, rating_function);
        }
        catch (const std::exception& ex) {
            res = false;
            descr += std::string("\nfailed with exception: ") + ex.what();
        }

        if (!res) {
            output << ss.str() << std::endl;
            UT_ASSERT(res, "Cannot build cluster topology: " << descr);
        }

        //Check topology
        output << "\n\n\n" << ss.str() << std::endl;

        std::vector<expected_indices_tuple> expected_seq;
        set_control_indices<ccl_scaleout_proxy<ccl_gpu_comm>>(
            expected_seq, 0, optional_indices{ true, { 3 }, { 0 } });
        set_control_indices<ccl_virtual_gpu_comm>(
            expected_seq, 0, optional_indices{ true, { 4, 5 }, { 1, 2 } });

        std::tie(res, descr) =
            check_ring_multiple_topologies<topology, class_id, process_group_context>(
                pg_comm->process_device_topology, params.thread_ids, expected_seq);
        if (!res) {
            output << "\nLOG:\n" << ss.str() << std::endl;
            output << descr;
            UT_ASSERT(res, descr);
        }
    }
}

TEST_F(router_fixture, cluster_simple_scaleup_scaleout_test) {
    using namespace native;
    using namespace native::detail;
    using namespace ::utils;

    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;

    {
        stub::make_stub_devices({ dev_0, dev_1, dev_2, dev_3 });

        size_t process_index = 1;
        process_creator_params params = prepare_process_params(
            process_index,
            // indices per threads in process_index
            {
                { 0, { dev_0, dev_0, dev_0 } },
            },
            // indices per processes in cluster
            {
                { 0, { dev_0, dev_0, dev_0 } },
                { process_index, { dev_1, dev_1, dev_1 } },
                { 2, { dev_2, dev_2, dev_2 } },
                { 3, { dev_3, dev_3, dev_3 } },
            },

            // place process index on differen node name
            { { 0, "node_0" }, { process_index, "node_0" }, { 2, "node_2" }, { 3, "node_2" } });

        // initial device connectivities matrix
        adjacency_matrix expected_matrix{
            { dev_0, { { dev_0, 1 }, { dev_1, 1 }, { dev_2, 1 }, { dev_3, 1 } } },
            { dev_1, { { dev_0, 1 }, { dev_1, 1 }, { dev_2, 1 }, { dev_3, 1 } } },
            { dev_2, { { dev_0, 1 }, { dev_1, 1 }, { dev_2, 1 }, { dev_3, 1 } } },
            { dev_3, { { dev_0, 1 }, { dev_1, 1 }, { dev_2, 1 }, { dev_3, 1 } } },
        };

        // stub for real device connectivity
        native::detail::p2p_rating_function rating_function =
            std::bind(::utils::test_custom_p2p_ping,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      expected_matrix);

        // stub for cluster device topology
        global_sorted_colored_plain_graphs control_cluster_topology{
            { 0,
              {
                  { { 0, dev_0 }, { 0, dev_0 }, { 0, dev_0 } },
              } },
            { process_index,
              {
                  { { process_index, dev_1 }, { process_index, dev_1 }, { process_index, dev_1 } },
              } },
            { 2,
              {
                  { { 2, dev_2 }, { 2, dev_2 }, { 2, dev_2 } },
              } },
            { 3,
              {
                  { { 3, dev_3 }, { 3, dev_3 }, { 3, dev_3 } },
              } }
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        // build device group context at first
        for (size_t thread_id : params.thread_ids) {
            const ccl::device_indices_type& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, rating_function);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        // skip thread group context
        // create cluster group context
        cluster_group_device_creator top(params.process_index,
                                         params.process_ids.size(),
                                         *pg_comm,
                                         *pg_comm->gpu_device_storage);

        std::stringstream ss;
        adjacency_matrix matrix = cluster_group_device_creator::build_p2p_capability_matrix(
            ss, params.total_node_mask, rating_function);
        //check matrix
        if (matrix != expected_matrix) {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix << "\nExpected matrix:\n"
                   << expected_matrix << std::endl;
            UT_ASSERT(false, "Matrix should be equal to expected_matrix");
        }
        ss.clear();

        // build test topology
        bool res = false;
        std::string descr;

        try {
            ccl::context_comm_addr comm_addr;
            res =
                top.build_all(ss, comm_addr, tg_comm->per_thread_indices, matrix, rating_function);
        }
        catch (const std::exception& ex) {
            res = false;
            descr += std::string("\nfailed with exception: ") + ex.what();
        }

        if (!res) {
            output << ss.str() << std::endl;
            UT_ASSERT(res, "Cannot build cluster topology: " << descr);
        }

        //Check topology
        output << "\n\n\n" << ss.str() << std::endl;

        std::vector<expected_indices_tuple> expected_seq;
        set_control_indices<ccl_scaleout_proxy<ccl_gpu_scaleup_proxy<ccl_gpu_comm>>>(
            expected_seq, 0, optional_indices{ true, { 3 }, { 0 } });
        set_control_indices<ccl_virtual_gpu_comm>(
            expected_seq, 0, optional_indices{ true, { 4, 5 }, { 1, 2 } });

        std::tie(res, descr) =
            check_ring_multiple_topologies<topology, class_id, process_group_context>(
                pg_comm->process_device_topology, params.thread_ids, expected_seq);
        if (!res) {
            output << "\nLOG:\n" << ss.str() << std::endl;
            output << descr;
            UT_ASSERT(res, descr);
        }
    }
}

/*************With NUMA**********************/
TEST_F(router_fixture, cluster_numa_scaleup_test) {
    using namespace native;
    using namespace native::detail;
    using namespace ::utils;

    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;

    {
        stub::make_stub_devices({ dev_0, dev_1, dev_2 });

        size_t process_index = 1;
        process_creator_params params = prepare_process_params(
            process_index,
            // indices per threads in process_index
            {
                { 0, { dev_0, dev_1, dev_2 } },
                { 1, { dev_3, dev_4, dev_5 } },
            },
            // indices per processes in cluster
            {
                { 0, { dev_0, dev_0, dev_0 } },
                { process_index, { dev_0, dev_1, dev_2, dev_3, dev_4, dev_5 } },
                { 2, { dev_5, dev_5, dev_5 } },
            });

        // initial device connectivities matrix
        adjacency_matrix expected_matrix{ { dev_0,
                                            { { dev_0, 1 },
                                              { dev_1, 1 },
                                              { dev_2, 1 },
                                              { dev_3, 0 },
                                              { dev_4, 0 },
                                              { dev_5, 0 } } },
                                          { dev_1,
                                            { { dev_0, 1 },
                                              { dev_1, 1 },
                                              { dev_2, 1 },
                                              { dev_3, 0 },
                                              { dev_4, 0 },
                                              { dev_5, 0 } } },
                                          { dev_2,
                                            { { dev_0, 1 },
                                              { dev_1, 1 },
                                              { dev_2, 1 },
                                              { dev_3, 0 },
                                              { dev_4, 0 },
                                              { dev_5, 0 } } },
                                          { dev_3,
                                            { { dev_0, 0 },
                                              { dev_1, 0 },
                                              { dev_2, 0 },
                                              { dev_3, 1 },
                                              { dev_4, 1 },
                                              { dev_5, 1 } } },
                                          { dev_4,
                                            { { dev_0, 0 },
                                              { dev_1, 0 },
                                              { dev_2, 0 },
                                              { dev_3, 1 },
                                              { dev_4, 1 },
                                              { dev_5, 1 } } },
                                          { dev_5,
                                            { { dev_0, 0 },
                                              { dev_1, 0 },
                                              { dev_2, 0 },
                                              { dev_3, 1 },
                                              { dev_4, 1 },
                                              { dev_5, 1 } } } };

        // stub for real device connectivity
        native::detail::p2p_rating_function rating_function =
            std::bind(::utils::test_custom_p2p_ping,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      expected_matrix);

        // stub for cluster device topology
        global_sorted_colored_plain_graphs control_cluster_topology{
            { 0,
              {
                  { { 0, dev_0 }, { 0, dev_0 }, { 0, dev_0 } },
              } },
            {
                process_index,
                { { { 1, dev_0 }, { 1, dev_1 }, { 1, dev_2 } },
                  { { 1, dev_3 }, { 1, dev_4 }, { 1, dev_5 } } },
            },
            { 2,
              {
                  { { 2, dev_5 }, { 2, dev_5 }, { 2, dev_5 } },
              } }
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        // build device group context at first
        for (size_t thread_id : params.thread_ids) {
            const ccl::device_indices_type& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, rating_function);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        // skip thread group context
        // create cluster group context
        cluster_group_device_creator top(params.process_index,
                                         params.process_ids.size(),
                                         *pg_comm,
                                         *pg_comm->gpu_device_storage);

        std::stringstream ss;
        adjacency_matrix matrix = cluster_group_device_creator::build_p2p_capability_matrix(
            ss, params.total_node_mask, rating_function);
        //check matrix
        if (matrix != expected_matrix) {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix << "\nExpected matrix:\n"
                   << expected_matrix << std::endl;
            UT_ASSERT(false, "Matrix should be equal to expected_matrix");
        }
        ss.clear();

        // build test topology
        bool res = false;
        std::string descr;

        try {
            ccl::context_comm_addr comm_addr;
            res =
                top.build_all(ss, comm_addr, tg_comm->per_thread_indices, matrix, rating_function);
        }
        catch (const std::exception& ex) {
            res = false;
            descr += std::string("\nfailed with exception: ") + ex.what();
        }

        if (!res) {
            output << ss.str() << std::endl;
            UT_ASSERT(res, "Cannot build cluster topology: " << descr);
        }

        //Check topology
        output << "\n\n\n" << ss.str() << std::endl;

        std::vector<expected_indices_tuple> expected_seq;
        set_control_indices<ccl_gpu_scaleup_proxy<ccl_gpu_comm>>(
            expected_seq, 0, optional_indices{ true, { 3 }, { 0 } });
        set_control_indices<ccl_gpu_comm>(expected_seq, 0, optional_indices{ true, { 4 }, { 1 } });
        set_control_indices<ccl_numa_proxy<ccl_gpu_comm>>(
            expected_seq, 0, optional_indices{ true, { 5 }, { 2 } });

        set_control_indices<ccl_gpu_comm>(
            expected_seq, 1, optional_indices{ true, { 6, 7 }, { 0, 1 } });
        set_control_indices<ccl_numa_proxy<ccl_gpu_comm>>(
            expected_seq, 1, optional_indices{ true, { 8 }, { 2 } });

        std::tie(res, descr) =
            check_ring_multiple_topologies<topology, class_id, process_group_context>(
                pg_comm->process_device_topology, params.thread_ids, expected_seq, true);
        if (!res) {
            output << "\nLOG:\n" << ss.str() << std::endl;
            output << descr;
            UT_ASSERT(res, descr);
        }
    }
}

TEST_F(router_fixture, cluster_numa_scaleout_test) {
    using namespace native;
    using namespace native::detail;
    using namespace ::utils;

    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;

    {
        stub::make_stub_devices({ dev_0, dev_1, dev_2 });

        size_t process_index = 1;
        process_creator_params params = prepare_process_params(
            process_index,
            // indices per threads in process_index
            {
                { 0, { dev_0, dev_1, dev_2 } },
                { 1, { dev_3, dev_4, dev_5 } },
            },
            // indices per processes in cluster
            {
                { 0, { dev_0, dev_0, dev_0 } },
                { process_index, { dev_0, dev_1, dev_2, dev_3, dev_4, dev_5 } },
                { 2, { dev_5, dev_5, dev_5 } },
            },
            // place process index on differen node name
            { { 0, "node_0" }, { process_index, "process_node" }, { 2, "node_2" } });

        // initial device connectivities matrix
        adjacency_matrix expected_matrix{ { dev_0,
                                            { { dev_0, 1 },
                                              { dev_1, 1 },
                                              { dev_2, 1 },
                                              { dev_3, 0 },
                                              { dev_4, 0 },
                                              { dev_5, 0 } } },
                                          { dev_1,
                                            { { dev_0, 1 },
                                              { dev_1, 1 },
                                              { dev_2, 1 },
                                              { dev_3, 0 },
                                              { dev_4, 0 },
                                              { dev_5, 0 } } },
                                          { dev_2,
                                            { { dev_0, 1 },
                                              { dev_1, 1 },
                                              { dev_2, 1 },
                                              { dev_3, 0 },
                                              { dev_4, 0 },
                                              { dev_5, 0 } } },
                                          { dev_3,
                                            { { dev_0, 0 },
                                              { dev_1, 0 },
                                              { dev_2, 0 },
                                              { dev_3, 1 },
                                              { dev_4, 1 },
                                              { dev_5, 1 } } },
                                          { dev_4,
                                            { { dev_0, 0 },
                                              { dev_1, 0 },
                                              { dev_2, 0 },
                                              { dev_3, 1 },
                                              { dev_4, 1 },
                                              { dev_5, 1 } } },
                                          { dev_5,
                                            { { dev_0, 0 },
                                              { dev_1, 0 },
                                              { dev_2, 0 },
                                              { dev_3, 1 },
                                              { dev_4, 1 },
                                              { dev_5, 1 } } } };

        // stub for real device connectivity
        native::detail::p2p_rating_function rating_function =
            std::bind(::utils::test_custom_p2p_ping,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      expected_matrix);

        // stub for cluster device topology
        global_sorted_colored_plain_graphs control_cluster_topology{
            { 0,
              {
                  { { 0, dev_0 }, { 0, dev_0 }, { 0, dev_0 } },
              } },
            {
                process_index,
                { { { 1, dev_0 }, { 1, dev_1 }, { 1, dev_2 } },
                  { { 1, dev_3 }, { 1, dev_4 }, { 1, dev_5 } } },
            },
            { 2,
              {
                  { { 2, dev_5 }, { 2, dev_5 }, { 2, dev_5 } },
              } }
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        // build device group context at first
        for (size_t thread_id : params.thread_ids) {
            const ccl::device_indices_type& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, rating_function);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        // skip thread group context
        // create cluster group context
        cluster_group_device_creator top(params.process_index,
                                         params.process_ids.size(),
                                         *pg_comm,
                                         *pg_comm->gpu_device_storage);

        std::stringstream ss;
        adjacency_matrix matrix = cluster_group_device_creator::build_p2p_capability_matrix(
            ss, params.total_node_mask, rating_function);
        //check matrix
        if (matrix != expected_matrix) {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix << "\nExpected matrix:\n"
                   << expected_matrix << std::endl;
            UT_ASSERT(false, "Matrix should be equal to expected_matrix");
        }
        ss.clear();

        // build test topology
        bool res = false;
        std::string descr;

        try {
            ccl::context_comm_addr comm_addr;
            res =
                top.build_all(ss, comm_addr, tg_comm->per_thread_indices, matrix, rating_function);
        }
        catch (const std::exception& ex) {
            res = false;
            descr += std::string("\nfailed with exception: ") + ex.what();
        }

        if (!res) {
            output << ss.str() << std::endl;
            UT_ASSERT(res, "Cannot build cluster topology: " << descr);
        }

        //Check topology
        output << "\n\n\n" << ss.str() << std::endl;

        std::vector<expected_indices_tuple> expected_seq;
        set_control_indices<ccl_scaleout_proxy<ccl_gpu_comm>>(
            expected_seq, 0, optional_indices{ true, { 3 }, { 0 } });
        set_control_indices<ccl_gpu_comm>(expected_seq, 0, optional_indices{ true, { 4 }, { 1 } });
        set_control_indices<ccl_numa_proxy<ccl_gpu_comm>>(
            expected_seq, 0, optional_indices{ true, { 5 }, { 2 } });

        set_control_indices<ccl_gpu_comm>(
            expected_seq, 1, optional_indices{ true, { 6, 7 }, { 0, 1 } });
        set_control_indices<ccl_numa_proxy<ccl_gpu_comm>>(
            expected_seq, 1, optional_indices{ true, { 8 }, { 2 } });

        std::tie(res, descr) =
            check_ring_multiple_topologies<topology, class_id, process_group_context>(
                pg_comm->process_device_topology, params.thread_ids, expected_seq, true);
        if (!res) {
            output << "\nLOG:\n" << ss.str() << std::endl;
            output << descr;
            UT_ASSERT(res, descr);
        }
    }
}

TEST_F(router_fixture, cluster_numa_scaleup_scale_out_test) {
    using namespace native;
    using namespace native::detail;
    using namespace ::utils;

    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;

    {
        stub::make_stub_devices({ dev_0, dev_1, dev_2 });

        size_t process_index = 1;
        process_creator_params params = prepare_process_params(
            process_index,
            // indices per threads in process_index
            {
                { 0, { dev_0, dev_1, dev_2 } },
                { 1, { dev_3, dev_4, dev_5 } },
            },
            // indices per processes in cluster
            { { 0, { dev_0, dev_0, dev_0 } },
              { process_index, { dev_0, dev_1, dev_2, dev_3, dev_4, dev_5 } },
              { 2, { dev_5, dev_5, dev_5 } },
              { 3, { dev_6, dev_6, dev_6 } } },
            // place process index on differen node name
            { { 0, "node_0" }, { process_index, "node_0" }, { 2, "node_2" }, { 3, "node_2" } });

        // initial device connectivities matrix
        adjacency_matrix expected_matrix{ { dev_0,
                                            { { dev_0, 1 },
                                              { dev_1, 1 },
                                              { dev_2, 1 },
                                              { dev_3, 0 },
                                              { dev_4, 0 },
                                              { dev_5, 0 },
                                              { dev_6, 0 } } },
                                          { dev_1,
                                            { { dev_0, 1 },
                                              { dev_1, 1 },
                                              { dev_2, 1 },
                                              { dev_3, 0 },
                                              { dev_4, 0 },
                                              { dev_5, 0 },
                                              { dev_6, 0 } } },
                                          { dev_2,
                                            { { dev_0, 1 },
                                              { dev_1, 1 },
                                              { dev_2, 1 },
                                              { dev_3, 0 },
                                              { dev_4, 0 },
                                              { dev_5, 0 },
                                              { dev_6, 0 } } },
                                          { dev_3,
                                            { { dev_0, 0 },
                                              { dev_1, 0 },
                                              { dev_2, 0 },
                                              { dev_3, 1 },
                                              { dev_4, 1 },
                                              { dev_5, 1 },
                                              { dev_6, 0 } } },
                                          { dev_4,
                                            { { dev_0, 0 },
                                              { dev_1, 0 },
                                              { dev_2, 0 },
                                              { dev_3, 1 },
                                              { dev_4, 1 },
                                              { dev_5, 1 },
                                              { dev_6, 0 } } },
                                          { dev_5,
                                            { { dev_0, 0 },
                                              { dev_1, 0 },
                                              { dev_2, 0 },
                                              { dev_3, 1 },
                                              { dev_4, 1 },
                                              { dev_5, 1 },
                                              { dev_6, 0 } } },
                                          { dev_6,
                                            { { dev_0, 0 },
                                              { dev_1, 0 },
                                              { dev_2, 0 },
                                              { dev_3, 0 },
                                              { dev_4, 0 },
                                              { dev_5, 0 },
                                              { dev_6, 1 } } } };

        // stub for real device connectivity
        native::detail::p2p_rating_function rating_function =
            std::bind(::utils::test_custom_p2p_ping,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      expected_matrix);

        // stub for cluster device topology
        global_sorted_colored_plain_graphs control_cluster_topology{
            { 0,
              {
                  { { 0, dev_0 }, { 0, dev_0 }, { 0, dev_0 } },
              } },
            {
                process_index,
                { { { process_index, dev_0 }, { process_index, dev_1 }, { process_index, dev_2 } },
                  { { process_index, dev_3 },
                    { process_index, dev_4 },
                    { process_index, dev_5 } } },
            },
            { 2,
              {
                  { { 2, dev_5 }, { 2, dev_5 }, { 2, dev_5 } },
              } },
            { 3,
              {
                  { { 3, dev_6 }, { 3, dev_6 }, { 3, dev_6 } },
              } }
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        // build device group context at first
        for (size_t thread_id : params.thread_ids) {
            const ccl::device_indices_type& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, rating_function);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        // skip thread group context
        // create cluster group context
        cluster_group_device_creator top(params.process_index,
                                         params.process_ids.size(),
                                         *pg_comm,
                                         *pg_comm->gpu_device_storage);

        std::stringstream ss;
        adjacency_matrix matrix = cluster_group_device_creator::build_p2p_capability_matrix(
            ss, params.total_node_mask, rating_function);
        //check matrix
        if (matrix != expected_matrix) {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix << "\nExpected matrix:\n"
                   << expected_matrix << std::endl;
            UT_ASSERT(false, "Matrix should be equal to expected_matrix");
        }
        ss.clear();

        // build test topology
        bool res = false;
        std::string descr;

        try {
            ccl::context_comm_addr comm_addr;
            res =
                top.build_all(ss, comm_addr, tg_comm->per_thread_indices, matrix, rating_function);
        }
        catch (const std::exception& ex) {
            res = false;
            descr += std::string("\nfailed with exception: ") + ex.what();
        }

        if (!res) {
            output << ss.str() << std::endl;
            UT_ASSERT(res, "Cannot build cluster topology: " << descr);
        }

        //Check topology
        output << "\n\n\n" << ss.str() << std::endl;

        std::vector<expected_indices_tuple> expected_seq;
        set_control_indices<ccl_scaleout_proxy<ccl_gpu_scaleup_proxy<ccl_gpu_comm>>>(
            expected_seq, 0, optional_indices{ true, { 3 }, { 0 } });
        set_control_indices<ccl_gpu_comm>(expected_seq, 0, optional_indices{ true, { 4 }, { 1 } });
        set_control_indices<ccl_numa_proxy<ccl_gpu_comm>>(
            expected_seq, 0, optional_indices{ true, { 5 }, { 2 } });

        set_control_indices<ccl_gpu_comm>(
            expected_seq, 1, optional_indices{ true, { 6, 7 }, { 0, 1 } });
        set_control_indices<ccl_numa_proxy<ccl_gpu_comm>>(
            expected_seq, 1, optional_indices{ true, { 8 }, { 2 } });

        std::tie(res, descr) =
            check_ring_multiple_topologies<topology, class_id, process_group_context>(
                pg_comm->process_device_topology, params.thread_ids, expected_seq, true);
        if (!res) {
            output << "\nLOG:\n" << ss.str() << std::endl;
            output << descr;
            UT_ASSERT(res, descr);
        }
    }
}

} // namespace topology_suite
