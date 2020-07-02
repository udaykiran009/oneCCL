#pragma once
#include <memory>
#include <vector>


#include "sched/entry/entry.hpp"
#include "native_device_api/l0/device.hpp"


/**
 * Add custom types support into native::memory example
 */
/*
namespace ccl
{
template<>
struct native_type_info<shared_event_float>
{
    static constexpr bool is_supported = true;
    static constexpr bool is_class = true;
};
}
#include "native_device_api/l0/primitives_impl.hpp"
*/
class ccl_gpu_sched;

template<class native_data_t>
class observer_event : public sched_entry
{
public:
    using element_t = native_data_t;
    using event_struct_t = typename shared_event_traits<element_t>::impl_t;
    using event_ptr_t = native::ccl_device::device_memory_ptr<event_struct_t>;

    observer_event(ccl_gpu_sched* parent,
                   native::ccl_device::device_memory_ptr<native_data_t> memory_producer,
                   native::ccl_device::device_memory_ptr<int> bytes_producer,
                   /*event_ptr_t device_event,*/
                   size_t wait_bytes_count);

    static constexpr const char* class_name()
    {
        return "OBSERVER_EVENT";
    }

    void start() override;
    void update() override;
    const char* name() const override;

private:
    bool consume_data();
    size_t bytes_consumed;
    size_t bytes_expected;

    native::ccl_device::device_memory_ptr<int> ready_bytes;
    native::ccl_device::device_memory_ptr<element_t> ready_memory_chunk;
    //event_ptr_t producer_event;
    std::vector<element_t> numa_host_data;// TODO
};
