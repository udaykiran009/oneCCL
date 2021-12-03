#pragma once

#include <atomic>
#include <unordered_map>

#include "atl/atl_base_comm.hpp"
#include "coll/algorithms/allreduce/allreduce_2d.hpp"
#include "common/comm/communicator_traits.hpp"
#include "common/comm/comm_interface.hpp"
#include "common/comm/comm_id_storage.hpp"
#include "common/comm/atl_tag.hpp"
#include "common/log/log.hpp"
#include "common/stream/stream.hpp"
#include "common/topology/topo_manager.hpp"
#include "common/utils/tree.hpp"
#include "common/utils/utils.hpp"
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/types_policy.hpp"
#include "oneapi/ccl/comm_split_attr_ids.hpp"
#include "oneapi/ccl/comm_split_attr_ids_traits.hpp"
#include "oneapi/ccl/comm_split_attr.hpp"
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/type_traits.hpp"
#include "oneapi/ccl/types_policy.hpp"
#include "oneapi/ccl/event.hpp"
#include "oneapi/ccl/coll_attr_ids.hpp"
#include "oneapi/ccl/coll_attr_ids_traits.hpp"
#include "oneapi/ccl/coll_attr.hpp"
#include "types_generator_defines.hpp"
#include "unordered_coll/unordered_coll.hpp"

// index = local_rank, value = global_rank
using ccl_rank2rank_map = std::vector<int>;

class ikvs_wrapper;

inline ccl_stream* get_stream_ptr(const ccl::stream::impl_value_t& stream) {
    if (stream.get() && stream->is_sycl_device_stream())
        return stream.get();
    else
        return nullptr;
}

using ccl_rank2rank_map = std::vector<int>;

class ccl_comm;
namespace ccl {
namespace v1 {
class kvs_interface;
}
} // namespace ccl

// the main purpose of internal comm is to hold
// shareable parts of ccl_comm which don't need to
// be copied/reset on ccl_comm's copy
class alignas(CACHELINE_SIZE) ccl_internal_comm {
public:
    ccl_internal_comm() = delete;
    ccl_internal_comm(const ccl_internal_comm& other) = delete;
    ccl_internal_comm& operator=(const ccl_internal_comm& other) = delete;

    ccl_internal_comm(int rank, int size, std::shared_ptr<atl_base_comm> atl_comm);
    ccl_internal_comm(const std::vector<int>& local_ranks,
                      int comm_size,
                      std::shared_ptr<ccl::kvs_interface> kvs_instance);

    ~ccl_internal_comm() = default;

    int rank() const noexcept {
        return m_rank;
    }

    int size() const noexcept {
        return m_size;
    }

    int pof2() const noexcept {
        return m_pof2;
    }

    const ccl_double_tree& dtree() const {
        return m_dtree;
    }

    void reset(int rank, int size);

    std::shared_ptr<atl_base_comm> atl_comm;
    std::unique_ptr<ccl_unordered_coll_manager> unordered_coll_manager;
    std::unique_ptr<ccl_allreduce_2d_builder> allreduce_2d_builder;

private:
    int m_rank;
    int m_size;
    int m_pof2;

    ccl_double_tree m_dtree;
};

class alignas(CACHELINE_SIZE) ccl_comm : public ccl::communicator_interface {
public:
    static constexpr int invalid_rank = -1;

    // maximum available number of active communicators
    static constexpr ccl_sched_id_t max_comm_count = std::numeric_limits<ccl_comm_id_t>::max();

    // maximum value of schedule id in scope of the current communicator
    static constexpr ccl_sched_id_t max_sched_count = std::numeric_limits<ccl_sched_id_t>::max();

    using traits = ccl::host_communicator_traits;

    // traits
    bool is_host() const noexcept override {
        return traits::is_host();
    }

    bool is_cpu() const noexcept override {
        return traits::is_cpu();
    }

    bool is_gpu() const noexcept override {
        return traits::is_gpu();
    }

    bool is_accelerator() const noexcept override {
        return traits::is_accelerator();
    }

    ccl_comm(ccl_comm_id_storage::comm_id&& id,
             std::shared_ptr<atl_base_comm> atl_comm,
             bool share_resources,
             bool is_sub_communicator);
    ccl_comm(std::shared_ptr<atl_base_comm> atl_comm,
             bool share_resources = false,
             bool is_sub_communicator = false);
    ccl_comm(int size,
             int rank,
             device_t device,
             context_t context,
             ccl::shared_ptr_class<ikvs_wrapper> kvs);
    ccl_comm(int size, int rank, ccl::shared_ptr_class<ikvs_wrapper> kvs);
    ccl_comm(int size, ccl::shared_ptr_class<ikvs_wrapper> kvs);
    ccl_comm();

    ccl_comm(ccl_comm& src) = delete;
    ccl_comm(ccl_comm&& src) = default;
    ccl_comm& operator=(ccl_comm& src) = delete;
    ccl_comm& operator=(ccl_comm&& src) = default;
    ~ccl_comm() = default;

private:
    // copy-constructor with explicit comm_id
    ccl_comm(const ccl_comm& src, ccl_comm_id_storage::comm_id&& id);

    ccl_comm* get_impl() {
        return this;
    }

    void create_sub_comms(std::shared_ptr<atl_base_comm> atl_comm);

public:
    ccl_comm* create_with_color(int color,
                                ccl_comm_id_storage* comm_ids,
                                bool share_resources) const;

    std::shared_ptr<ccl_comm> clone_with_new_id(ccl_comm_id_storage::comm_id&& id);

    void allocate_resources();

    ccl::communicator_interface_ptr split(const ccl::comm_split_attr& attr) override;

    std::string to_string() const;
    std::string to_string_ext() const;

    /**
     * Returns the number of @c rank in the global communicator
     * @param rank a rank which is part of the current communicator
     * @return number of @c rank in the global communicator
     */
    int get_global_rank(int rank, bool only_global = false) const;

    int get_rank_from_global(int global_rank) const;
    ccl_sched_id_t get_sched_id(bool use_internal_space);

    device_ptr_t get_device() const override {
        return device_ptr;
    }

    context_ptr_t get_context() const override {
        return context_ptr;
    }

    std::shared_ptr<atl_base_comm> get_atl_comm() const {
        return comm_impl->atl_comm;
    }

    std::shared_ptr<ccl_comm> get_r2r_comm() const {
        return r2r_comm;
    }

    std::shared_ptr<ccl_comm> get_node_comm() const {
        return node_comm;
    }

    std::shared_ptr<ccl_comm> get_even_comm() const {
        return even_comm;
    }

    std::shared_ptr<ccl_comm> get_pair_comm() const {
        return pair_comm;
    }

    const ccl_rank2rank_map& get_local2global_map() const {
        return local2global_map;
    }

    std::unique_ptr<ccl_unordered_coll_manager>& get_unordered_coll_manager() const {
        return comm_impl->unordered_coll_manager;
    }
    std::unique_ptr<ccl_allreduce_2d_builder>& get_allreduce_2d_builder() const {
        return comm_impl->allreduce_2d_builder;
    }

    int rank() const override {
        return comm_rank;
    }

    int size() const override {
        return comm_size;
    }

    int pof2() const noexcept {
        return comm_impl->pof2();
    }

    ccl_comm_id_t id() const noexcept {
        return comm_id->value();
    }

    const ccl_double_tree& dtree() const {
        return comm_impl->dtree();
    }

    // collectives operation declarations
    ccl::event barrier(const ccl::stream::impl_value_t& stream,
                       const ccl::barrier_attr& attr,
                       const ccl::vector_class<ccl::event>& deps = {}) override;
    ccl::event barrier_impl(const ccl::stream::impl_value_t& stream,
                            const ccl::barrier_attr& attr,
                            const ccl::vector_class<ccl::event>& deps = {});

    COMM_INTERFACE_COLL_METHODS(DEFINITION);
#ifdef CCL_ENABLE_SYCL
    SYCL_COMM_INTERFACE_COLL_METHODS(DEFINITION);
#endif // CCL_ENABLE_SYCL

    COMM_IMPL_DECLARATION;
    COMM_IMPL_CLASS_DECLARATION
    COMM_IMPL_SPARSE_DECLARATION;
    COMM_IMPL_SPARSE_CLASS_DECLARATION;

private:
    // this is an internal part of the communicator
    // we store there only the fields which should be shared
    // across ccl_comm copies/clones
    // everything else must go to ccl_comm
    std::shared_ptr<ccl_internal_comm> comm_impl;

    // ccl::device/context hasn't got a default c-tor
    // that's why we use shared_ptr<ccl::device/context>
    device_ptr_t device_ptr;
    context_ptr_t context_ptr;

    // TODO: double check if these can be moved to comm_impl as shared fields
    std::shared_ptr<ccl_comm> r2r_comm;
    std::shared_ptr<ccl_comm> node_comm;
    std::shared_ptr<ccl_comm> even_comm;
    std::shared_ptr<ccl_comm> pair_comm;
    ccl::comm_split_attr split_attr;

    // these fields are duplicate with the ones in ccl_internal_comm
    // but having them here allows to get them without going
    // through the shared_ptr indirection
    int comm_rank;
    int comm_size;

    ccl_rank2rank_map local2global_map{};
    ccl::topo_manager topo_manager;

    // comm_id is not default constructible but ccl_comm is, so use unique_ptr here
    std::unique_ptr<ccl_comm_id_storage::comm_id> comm_id;
    ccl_sched_id_t next_sched_id_internal;
    ccl_sched_id_t next_sched_id_external;

}; // class ccl_comm
