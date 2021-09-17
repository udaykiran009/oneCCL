#include "atl/atl_ofi_comm.hpp"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable_simple.h"
#include "atl/util/pm/pmi_rt/pmi_simple.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable/kvs/internal_kvs.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable_simple_internal.h"
#include "atl/ofi/atl_ofi.hpp"
#include "exec/exec.hpp"

static std::list<std::shared_ptr<atl_ofi>> transports{};

atl_ofi_comm::~atl_ofi_comm() {
    static std::mutex memory_mutex;
    std::lock_guard<std::mutex> lock(memory_mutex);
    transports.remove(transport);
    tag.reset();
}

atl_ofi_comm::atl_ofi_comm() {
    char* pm_type_str = getenv(PM_TYPE);

    if (pm_type_str) {
        if (strstr(pm_type_str, PM_RT_VAL_SIMPLE)) {
            pmi = std::unique_ptr<ipmi>(new pmi_simple());
        }
        else if (strstr(pm_type_str, PM_RT_VAL_RESIZABLE)) {
            std::shared_ptr<ikvs_wrapper> k(new internal_kvs());
            pmi = std::unique_ptr<ipmi>(new pmi_resizable(k));
        }
        else {
            LOG_ERROR("Unknown %s: %s\n", PM_TYPE, pm_type_str);
        }
    }
    else {
        pmi = std::unique_ptr<ipmi>(new pmi_simple());
    }
    transport = std::shared_ptr<atl_ofi>(new atl_ofi());

    init_transport();
}

atl_ofi_comm::atl_ofi_comm(std::shared_ptr<ikvs_wrapper> k) {
    char* pm_type_str = getenv(PM_TYPE);

    if (pm_type_str) {
        if (strstr(pm_type_str, PM_RT_VAL_SIMPLE)) {
            pmi = std::unique_ptr<ipmi>(new pmi_simple());
        }
        else if (strstr(pm_type_str, PM_RT_VAL_RESIZABLE)) {
            pmi = std::unique_ptr<ipmi>(new pmi_resizable(k));
        }
        else {
            LOG_ERROR("Unknown %s: %s\n", PM_TYPE, pm_type_str);
        }
    }
    else {
        pmi = std::unique_ptr<ipmi>(new pmi_simple());
    }
    transport = std::shared_ptr<atl_ofi>(new atl_ofi());

    init_transport();
}

atl_ofi_comm::atl_ofi_comm(int total_rank_count,
                           const std::vector<int>& ranks,
                           std::shared_ptr<ikvs_wrapper> k) {
    size_t transorts_count = transports.size();
    std::shared_ptr<internal_kvs> kvs;
    if ((kvs = std::dynamic_pointer_cast<internal_kvs>(k)) != nullptr) {
        pmi =
            std::unique_ptr<ipmi>(new pmi_resizable_simple_internal(total_rank_count, ranks, kvs));
    }
    else {
        pmi = std::unique_ptr<ipmi>(new pmi_resizable_simple(total_rank_count, ranks, k));
    }

    if (pmi->get_local_thread_idx() == 0) {
        transports.push_back(std::shared_ptr<atl_ofi>(new atl_ofi()));
    }
    //TODO: Rework it on barrier
    while (transorts_count == transports.size()) {
        ccl_yield(ccl::global_data::env().yield_type);
    }
    static std::mutex memory_mutex;
    {
        std::lock_guard<std::mutex> lock(memory_mutex);
        transport = transports.back();
    }

    init_transport();
}
void atl_ofi_comm::init_transport() {
    LOG_DEBUG("init ATL, requested ep_count ", attr.in.ep_count);
    static std::mutex memory_mutex;
    {
        std::lock_guard<std::mutex> lock(memory_mutex);
        if (!transport->is_inited()) {
            CCL_THROW_IF_NOT(
                transport->init(nullptr, nullptr, &attr, nullptr, pmi) == ATL_STATUS_SUCCESS,
                "failed to initialize ATL");
        }
    }
    eps = transport->get_eps();

    threads_per_process = pmi->get_threads_per_process();
    ranks_per_process = pmi->get_ranks_per_process();
    rank = pmi->get_rank();
    size = pmi->get_size();

    init_tag();

    if (pmi->get_local_thread_idx() == 0) {
        executor_update();
    }
}