#pragma once
#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)

#include "common/comm/single_device_communicator/single_device_base.hpp"
#include "common/comm/comm.hpp"
#include "common/comm/usm_visitor/usm_visitors.hpp"

namespace native {
struct device_group_context;
}

class single_device_communicator
        : public typed_single_device_base_communicator<single_device_communicator,
                                                       ccl::gpu_communicator_traits> {
public:
    using base_t = typed_single_device_base_communicator<single_device_communicator,
                                                         ccl::gpu_communicator_traits>;

    single_device_communicator(ccl::unified_device_type&& device,
                               ccl::unified_context_type&& context,
                               size_t thread_idx,
                               size_t proces_idx,
                               const ccl::comm_split_attr& attr);

    ~single_device_communicator();

    std::shared_ptr<ccl::communicator_interface> split(const ccl::comm_split_attr& attr) override;

#ifdef MULTI_GPU_SUPPORT
    void visit(ccl::gpu_comm_attr& comm_attr) override;
#endif
    ccl::event barrier(const ccl::stream::impl_value_t& op_stream,
                       const ccl::barrier_attr& attr,
                       const ccl::vector_class<ccl::event>& deps) override;

    COMM_IMPL_DECLARATION
    COMM_IMPL_CLASS_DECLARATION
    COMM_IMPL_SPARSE_DECLARATION
    COMM_IMPL_SPARSE_CLASS_DECLARATION

    void set_ccl_comm(std::shared_ptr<ccl_comm> imp);

    //TODO use visit() to set `context`
    void set_context(const ccl::unified_context_type::ccl_native_t& context);
    void set_context(const ccl::context& context);
private:
    std::shared_ptr<ccl_comm> comm_impl;
};

#endif //#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)
