#include "atl/util/pm/pmi_resizable_rt/pmi_resizable_simple.h"
#include "atl/util/pm/pmi_rt/pmi_simple.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable/kvs/internal_kvs.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable.h"
#include "atl/ofi/atl_ofi.h"
#include "atl/mpi/atl_mpi.h"
#include "atl_wrapper.h"
#include "common/global/global.hpp"

static std::list<std::shared_ptr<iatl>> transports{};

atl_attr_t atl_wrapper::attr = {
    1, /* ep_count */
    1, /* enable_shm */
    64, /* tag_bits */
    0xFFFFFFFFFFFFFFFF, /* max_tag */
    0, /* enable_rma */
    0 /* max_order_waw_size */
};

void atl_wrapper::set_internal_env(atl_attr_t& attr)
{
    auto transport_type = ccl::global_data::env().atl_transport;

    if (transport_type == ccl_atl_mpi)
        atl_mpi::atl_set_env(&attr);
    else if (transport_type == ccl_atl_ofi)
        atl_ofi::atl_set_env(&attr);
}

atl_wrapper::atl_wrapper() {

    auto transport_type = ccl::global_data::env().atl_transport;

    char* pm_type_str;
    switch (transport_type)
    {
        case ccl_atl_ofi:
            pm_type_str = getenv(PM_TYPE);
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
            transport = std::shared_ptr<iatl>(new atl_ofi());
            break;
        case ccl_atl_mpi:
            transport = std::shared_ptr<iatl>(new atl_mpi());
            break;
        default:
            LOG_ERROR("Unsupported yet");
            break;
    }

    init_transport();
}

atl_wrapper::atl_wrapper(std::shared_ptr<ikvs_wrapper> k) {

    auto transport_type = ccl::global_data::env().atl_transport;

    char* pm_type_str;
    switch (transport_type)
    {
        case ccl_atl_ofi:
            pm_type_str = getenv(PM_TYPE);
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
            transport = std::shared_ptr<iatl>(new atl_ofi());
            break;
        case ccl_atl_mpi:
            transport = std::shared_ptr<iatl>(new atl_mpi());
            break;
        default:
            LOG_ERROR("Unsupported yet");
            break;
    }

    init_transport();
}

atl_wrapper::atl_wrapper(size_t dev_count,
                         const std::vector<size_t> &ranks,
                         std::shared_ptr<ikvs_wrapper> k) {
    auto transport_type = ccl::global_data::env().atl_transport;

    switch (transport_type)
    {
        case ccl_atl_ofi:
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
            break;
        case ccl_atl_mpi:
             transport = std::shared_ptr<iatl>(new atl_mpi());
             break;
        default:
            LOG_ERROR("Unsupported yet");
            break;
    }

    init_transport();
}
void atl_wrapper::init_transport() {

    transport->atl_init(nullptr, nullptr, &attr, nullptr, pmi);
    eps = transport->atl_get_eps();
    tag = std::unique_ptr<ccl_atl_tag>(new ccl_atl_tag(attr.tag_bits, attr.max_tag));

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

    if (rank == 0)
        tag->print();
}
atl_wrapper::~atl_wrapper() {
    static std::mutex memory_mutex;
    std::lock_guard<std::mutex> lock(memory_mutex);
    transports.remove(transport);
    tag.reset();
}
