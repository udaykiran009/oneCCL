#include "atl/util/pm/pmi_resizable_rt/pmi_resizable_simple.h"
#include "atl/util/pm/pmi_rt/pmi_simple.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable/kvs/internal_kvs.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable.h"
#include "atl/ofi/atl_ofi.h"
#include "atl/mpi/atl_mpi.h"
#include "atl_wrapper.h"
#include "common/global/global.hpp"

static std::list<std::shared_ptr<iatl>> transports{};

atl_attr_t atl_wrapper::atl_attr = {
    1, /* ep_count */
    1, /* enable_shm */
    64, /* tag_bits */
    0xFFFFFFFFFFFFFFFF, /* max_tag */
    0, /* enable_rma */
    0 /* max_order_waw_size */
};

atl_wrapper::atl_wrapper() {
    auto transport_name = ccl::env_data::str_by_enum(ccl::env_data::atl_transport_names,
                                                     ccl::global_data::env().atl_transport);

    if (!transport_name.compare("ofi")) {
        char *type_str = getenv(PM_TYPE);

        if (type_str) {
            if (strstr(type_str, PM_RT_VAL_SIMPLE)) {
                pmi = std::unique_ptr<ipmi>(new pmi_simple());
            }
            else if (strstr(type_str, PM_RT_VAL_RESIZABLE)) {
                std::shared_ptr<ikvs_wrapper> k(new internal_kvs());
                pmi = std::unique_ptr<ipmi>(new pmi_resizable(k));
            }
            else {
                LOG_ERROR("Unknown %s: %s\n", PM_TYPE, type_str);
            }
        }
        else {
            pmi = std::unique_ptr<ipmi>(new pmi_simple());
        }
        transport = std::shared_ptr<iatl>(new atl_ofi());
    }
    else if (!transport_name.compare("mpi")) {
        transport = std::shared_ptr<iatl>(new atl_mpi());
    }
    else {
        LOG_ERROR("Unsupported yet");
    }
    init_transport();
}

atl_wrapper::atl_wrapper(std::shared_ptr<ikvs_wrapper> k) {
    auto transport_name = ccl::env_data::str_by_enum(ccl::env_data::atl_transport_names,
                                                     ccl::global_data::env().atl_transport);

    if (!transport_name.compare("ofi")) {
        char *type_str = getenv(PM_TYPE);

        if (type_str) {
            if (strstr(type_str, PM_RT_VAL_SIMPLE)) {
                pmi = std::unique_ptr<ipmi>(new pmi_simple());
            }
            else if (strstr(type_str, PM_RT_VAL_RESIZABLE)) {
                pmi = std::unique_ptr<ipmi>(new pmi_resizable(k));
            }
            else {
                LOG_ERROR("Unknown %s: %s\n", PM_TYPE, type_str);
            }
        }
        else {
            pmi = std::unique_ptr<ipmi>(new pmi_simple());
        }
        transport = std::shared_ptr<iatl>(new atl_ofi());
    }
    else if (!transport_name.compare("mpi")) {
        transport = std::shared_ptr<iatl>(new atl_mpi());
    }
    else {
        LOG_ERROR("Unsupported yet");
    }
    init_transport();
}

atl_wrapper::atl_wrapper(size_t dev_count,
                         const std::vector<size_t> &ranks,
                         std::shared_ptr<ikvs_wrapper> k) {
    auto transport_name = ccl::env_data::str_by_enum(ccl::env_data::atl_transport_names,
                                                     ccl::global_data::env().atl_transport);

    if (!transport_name.compare("ofi")) {
        pmi = std::unique_ptr<ipmi>(new pmi_resizable_simple(dev_count, ranks, k));

        if (pmi->get_thread() == 0) {
            transports.push_back(std::shared_ptr<iatl>(new atl_ofi()));
        }
        pmi->pmrt_barrier();
        static std::mutex memory_mutex;
        {
            std::lock_guard<std::mutex> lock(memory_mutex);
            transport = transports.back();
        }
    }
    else if (!transport_name.compare("mpi")) {
        transport = std::shared_ptr<iatl>(new atl_mpi());
    }
    else {
        LOG_ERROR("Unsupported yet");
    }
    init_transport();
}
void atl_wrapper::init_transport() {
    transport->atl_init(nullptr, nullptr, &atl_attr, nullptr, pmi);
    eps = transport->atl_get_eps();

    if (pmi) {
        threads_count = pmi->get_threads_count();
        devices_per_rank_count = pmi->get_devices_per_rank_count();
        rank = pmi->get_rank();
        size = pmi->get_size();
    }
    else {
        threads_count = 1;
        devices_per_rank_count = 1;
        rank = static_cast<atl_mpi *>(transport.get())->get_rank();
        size = static_cast<atl_mpi *>(transport.get())->get_size();
    }
}
atl_wrapper::~atl_wrapper() {
    static std::mutex memory_mutex;
    std::lock_guard<std::mutex> lock(memory_mutex);
    transports.remove(transport);
}
