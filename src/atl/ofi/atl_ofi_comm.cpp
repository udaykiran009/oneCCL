#include "atl/ofi/atl_ofi_comm.hpp"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable_simple.h"
#include "atl/util/pm/pmi_rt/pmi_simple.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable/kvs/internal_kvs.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable_simple_internal.h"
#include "atl/ofi/atl_ofi.hpp"
#include "exec/exec.hpp"

std::atomic<size_t> atl_ofi_comm::comm_count{ 0 };
atl_ofi* atl_ofi_comm::transport{ nullptr };

atl_ofi_comm::~atl_ofi_comm() {
    static std::mutex memory_mutex;
    std::lock_guard<std::mutex> lock(memory_mutex);
    tag.reset();
    comm_count--;
    if (comm_count.load() == 0) {
        delete transport;
        transport = nullptr;
    }
}

atl_ofi_comm::atl_ofi_comm() {
    char* pm_type_str = getenv(PM_TYPE);

    if (pm_type_str) {
        if (strstr(pm_type_str, PM_RT_VAL_SIMPLE)) {
            pmi = std::shared_ptr<ipmi>(new pmi_simple());
        }
        else if (strstr(pm_type_str, PM_RT_VAL_RESIZABLE)) {
            std::shared_ptr<ikvs_wrapper> k(new internal_kvs());
            pmi = std::shared_ptr<ipmi>(new pmi_resizable(k));
        }
        else {
            LOG_ERROR("Unknown %s: %s\n", PM_TYPE, pm_type_str);
        }
    }
    else {
        pmi = std::shared_ptr<ipmi>(new pmi_simple());
    }

    CCL_THROW_IF_NOT(init_transport(true) == ATL_STATUS_SUCCESS, "init transport failed");
}

atl_ofi_comm::atl_ofi_comm(std::shared_ptr<ikvs_wrapper> k) {
    char* pm_type_str = getenv(PM_TYPE);

    if (pm_type_str) {
        if (strstr(pm_type_str, PM_RT_VAL_SIMPLE)) {
            pmi = std::shared_ptr<ipmi>(new pmi_simple());
        }
        else if (strstr(pm_type_str, PM_RT_VAL_RESIZABLE)) {
            pmi = std::shared_ptr<ipmi>(new pmi_resizable(k));
        }
        else {
            LOG_ERROR("Unknown %s: %s\n", PM_TYPE, pm_type_str);
        }
    }
    else {
        pmi = std::shared_ptr<ipmi>(new pmi_simple());
    }

    CCL_THROW_IF_NOT(init_transport(true) == ATL_STATUS_SUCCESS, "init transport failed");
}

atl_ofi_comm::atl_ofi_comm(int comm_size,
                           const std::vector<int>& ranks,
                           std::shared_ptr<ikvs_wrapper> k) {
    std::shared_ptr<internal_kvs> kvs;
    if ((kvs = std::dynamic_pointer_cast<internal_kvs>(k)) != nullptr) {
        pmi = std::shared_ptr<ipmi>(new pmi_resizable_simple_internal(comm_size, ranks, kvs));
    }
    else {
        pmi = std::shared_ptr<ipmi>(new pmi_resizable_simple(comm_size, ranks, k));
    }

    CCL_THROW_IF_NOT(init_transport(true) == ATL_STATUS_SUCCESS, "init transport failed");
}
atl_status_t atl_ofi_comm::init_transport(bool is_new) {
    LOG_DEBUG("init ATL, requested ep_count ", attr.in.ep_count);

    if (is_new) {
        ATL_CHECK_STATUS(pmi->pmrt_init(), "pmi init failed");
        static std::mutex memory_mutex;
        {
            std::lock_guard<std::mutex> lock(memory_mutex);
            if (!transport) {
                transport = new atl_ofi();
            }
            if (!transport->is_inited()) {
                CCL_THROW_IF_NOT(
                    transport->init(nullptr, nullptr, &attr, nullptr, pmi) == ATL_STATUS_SUCCESS,
                    "failed to initialize ATL");

                if (pmi->get_rank() == 0) {
                    print_atl_attrs();
                }
            }
        }
        eps = transport->get_eps();
        coord = *transport->get_proc_coord();

        parent_rank = rank = pmi->get_rank();
        parent_size = size = pmi->get_size();

        rank2rank_map.resize(size);
        transport->get_rank2rank_map(pmi, rank2rank_map);
    }

    threads_per_process = pmi->get_threads_per_process();
    ranks_per_process = pmi->get_ranks_per_process();
    comm_count++;

    init_tag();

    if (pmi->get_local_thread_idx() == 0) {
        executor_update();
    }

    return ATL_STATUS_SUCCESS;
}

std::shared_ptr<atl_base_comm> atl_ofi_comm::comm_split(int color) {
    return std::shared_ptr<atl_base_comm>(new atl_ofi_comm(this, color));
}

atl_ofi_comm::atl_ofi_comm(atl_ofi_comm* parent, int color) {
    eps = parent->eps;
    parent_size = parent->parent_size;
    parent_rank = parent->parent_rank;
    pmi = parent->pmi;

    coord.hostname_hash = transport->get_proc_coord()->hostname_hash;
    coord.local_count = 0;
    coord.local_idx = 0;

    std::vector<rank_info_t> ranks_info(parent_size);
    rank_info_t rank_info{ color, parent_rank, coord.hostname_hash };
    std::vector<int> recv_lens(parent_size, sizeof(rank_info));
    std::vector<int> offsets(parent_size);
    offsets[0] = 0;
    for (size_t i = 1; i < offsets.size(); i++) {
        offsets[i] = offsets[i - 1] + recv_lens[i];
    }
    atl_req req;

    parent->allgatherv(0 /* ep_idx */,
                       &rank_info,
                       sizeof(rank_info),
                       ranks_info.data(),
                       recv_lens.data(),
                       offsets.data(),
                       &req);

    size = 0;
    for (auto& it : ranks_info) {
        int recv_color;
        int recv_rank;
        size_t recv_hash;
        std::tie(recv_color, recv_rank, recv_hash) = it;
        if (recv_color == color) {
            rank2rank_map.push_back(recv_rank);

            if (recv_hash == coord.hostname_hash) {
                coord.local_count++;
            }
            if (recv_rank == parent_rank) {
                coord.global_idx = rank = size;
                coord.local_idx = coord.local_count;
            }
            size++;
        }
    }
    coord.global_count = size;
    CCL_THROW_IF_NOT(init_transport(false) == ATL_STATUS_SUCCESS, "init transport failed");
}

atl_status_t atl_ofi_comm::allgatherv(size_t ep_idx,
                                      const void* send_buf,
                                      size_t send_len,
                                      void* recv_buf,
                                      const int* recv_lens,
                                      const int* offsets,
                                      atl_req_t* req) {
    std::vector<atl_req> send_reqs(size - 1);
    std::vector<atl_req> recv_reqs(size - 1);

    const int tag_coef = 1000;
    for (int i = 0, j = 0; i < size; i++) {
        if (i == rank)
            continue;
        atl_status_t ret;
        do {
            /* TODO: rework tag initialization */
            ret = send(ep_idx, send_buf, send_len, i, rank * tag_coef + i, &(send_reqs[j]));
            CCL_THROW_IF_NOT(ret != ATL_STATUS_FAILURE, "send failed");
            ccl_yield(ccl::global_data::env().yield_type);
        } while (ret == ATL_STATUS_AGAIN);

        do {
            ret = recv(ep_idx,
                       (char*)recv_buf + offsets[i],
                       recv_lens[i],
                       i,
                       i * tag_coef + rank,
                       &(recv_reqs[j]));
            CCL_THROW_IF_NOT(ret != ATL_STATUS_FAILURE, "recv failed");
            ccl_yield(ccl::global_data::env().yield_type);
        } while (ret == ATL_STATUS_AGAIN);
        j++;
    }

    if ((char*)recv_buf + offsets[rank] != send_buf) {
        memcpy((char*)recv_buf + offsets[rank], send_buf, recv_lens[rank]);
    }
    bool is_completed = false;
    while (!is_completed) {
        is_completed = true;
        poll(ep_idx);
        for (size_t i = 0; i < send_reqs.size(); i++) {
            if (!send_reqs[i].is_completed) {
                CCL_THROW_IF_NOT(check(ep_idx, &(send_reqs[i])) != ATL_STATUS_FAILURE,
                                 "check send failed");
                is_completed = false;
            }
            if (!recv_reqs[i].is_completed) {
                CCL_THROW_IF_NOT(check(ep_idx, &(recv_reqs[i])) != ATL_STATUS_FAILURE,
                                 "check recv failed");
                is_completed = false;
            }
        }
    }
    atl_ofi_req_t* ofi_req;

    req->is_completed = false;
    ofi_req = ((atl_ofi_req_t*)req->internal);
    ofi_req->comp_state = ATL_OFI_COMP_COMPLETED;
    return ATL_STATUS_SUCCESS;
}
