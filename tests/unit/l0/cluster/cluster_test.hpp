#define private public
#define protected public
#include "common/comm/l0/context/process_group_ctx.hpp"
#undef protected
#undef private

#if 0
namespace cluster_suite {
TEST_F(gpu_aggregator_fixture, DISABLED_host_mask_validity) {
    using namespace native;

    ccl::cluster_aggregated_device_mask_t cluster_mask;
    ccl::process_aggregated_device_mask_t allied_processes_mask;
    bool check;
    std::string err_descr;

    std::string case_name = "validHostMask";
    cluster_mask.insert(
        { case_name, { { 0, ccl::device_mask_t("0011") }, { 1, ccl::device_mask_t("1100") } } });
    allied_processes_mask = cluster_mask[case_name];
    std::tie(check, err_descr) = std::make_tuple<bool, std::string>(
        true,
        "TODO"); //process_group_context::check_device_mask_validity_across_allied_processes(allied_processes_mask);
    ASSERT_TRUE(check) << case_name << ", Error: " << err_descr << std::endl;

    case_name = "duplicatedIndicesMask";
    cluster_mask.insert({ case_name,
                          { { 0, ccl::device_mask_t("0011") },
                            { 1, ccl::device_mask_t("0110") },
                            { 2, ccl::device_mask_t("1100") } } });
    allied_processes_mask = cluster_mask[case_name];
    std::tie(check, err_descr) = std::make_tuple<bool, std::string>(
        true,
        "TODO"); //process_group_context::check_device_mask_validity_across_allied_processes(allied_processes_mask);
    std::cerr << case_name << ", Error: " << err_descr << std::endl;
    ASSERT_FALSE(check) << case_name << ", Error: " << err_descr << std::endl;
}

TEST_F(communicator_fixture, build_allied_processes_affinity_mask) {
    using namespace native;
    size_t proces_idx = get_fixture_rank();
    size_t size = get_fixture_size();
    (void)size;
    ccl::process_device_indices_type process_mask{
        { 0,
          { ccl::device_index_type(0, 0, ccl::unused_index_value),
            ccl::device_index_type(0, 0, ccl::unused_index_value),
            ccl::device_index_type(0, 1, ccl::unused_index_value),
            ccl::device_index_type(0, 1, ccl::unused_index_value) } },
        { 1,
          { ccl::device_index_type(0, 1, ccl::unused_index_value),
            ccl::device_index_type(0, 1, ccl::unused_index_value),
            ccl::device_index_type(0, 0, ccl::unused_index_value),
            ccl::device_index_type(0, 0, ccl::unused_index_value) } }
    };

    auto ccl_comm = get_fixture_comm();
    {
        output << "One node case" << std::endl;
        process_group_context p_group_comm(ccl_comm);

        UT_ASSERT(!p_group_comm.get_host_id().empty(), "Hostname is empty");
        ccl::cluster_device_indices_type cluster_mask;
        std::string case_name = p_group_comm.get_host_id();

        cluster_mask.insert({ case_name,
                              ccl::process_device_indices_type{
                                  { proces_idx, process_mask.find(proces_idx)->second } } });

        output << "initialize global mask for host: " << p_group_comm.get_host_id()
               << ", process idx: " << proces_idx << std::endl;
        initialize_global_mask(cluster_mask);

        process_group_context::dump_cluster_affinity_indices(cluster_mask, output);
        p_group_comm.build_cluster_affinity_table(get_process_mask(case_name, proces_idx));

        const auto& node_mask = p_group_comm.get_node_afinity_indices(p_group_comm.get_host_id());
        /*UT_ASSERT(node_mask.size() == p_group_comm.size(), "One node case invalid processes affinity masks: " << node_mask.size()
                                                           << ", expected: " <<  p_group_comm.size());
                                                           * */
        for (const auto& val : node_mask) {
            UT_ASSERT(val.first < process_mask.size(),
                      "Invalid process id: " << val.first
                                             << ", should be less than: " << process_mask.size());
            UT_ASSERT(val.second == process_mask[val.first], "Invalid mask: " /*<< val.second*/
                                                             << " for process id: " << val.first/*
                                                             << ", expected: " << process_mask[val.first]*/);
        }
    }

    {
        output << "Two node case" << std::endl;
        process_group_context p_group_comm(ccl_comm->get_impl());

        std::string real_hostname = p_group_comm.get_host_id();
        if (proces_idx == 0) {
            p_group_comm.my_host_name = "ZeroRankHostname";
        }

        UT_ASSERT(!p_group_comm.get_host_id().empty(), "Hostname is empty");
        ccl::cluster_device_indices_type cluster_mask;
        std::string case_name = p_group_comm.get_host_id();

        cluster_mask.insert({ case_name,
                              ccl::process_device_indices_type{
                                  { proces_idx, process_mask.find(proces_idx)->second } } });

        output << "initialize global mask for host: " << p_group_comm.get_host_id()
               << ", process idx: " << proces_idx << std::endl;
        initialize_global_mask(cluster_mask);
        process_group_context::dump_cluster_affinity_indices(cluster_mask, output);
        p_group_comm.build_cluster_affinity_table(get_process_mask(case_name, proces_idx));

        const auto& node_mask = p_group_comm.get_node_afinity_indices(p_group_comm.get_host_id());
        UT_ASSERT(node_mask.size() == 1,
                  "Invalid nodes count in affinity : " << node_mask.size() << ", expected: " << 1);
        const auto& val = node_mask.find(proces_idx);
        UT_ASSERT(val != node_mask.end(), "Own rank is not found in nodes maks");
        UT_ASSERT(val->second == process_mask[proces_idx], "Invalid mask: "/* << val->second*/
                                                         << " for process id: " << val->first/*
                                                         << ", expected: " << process_mask[proces_idx]*/);
    }
}
} // namespace cluster_suite
#endif
