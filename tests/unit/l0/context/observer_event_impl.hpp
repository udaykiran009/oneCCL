#pragma once

#include "observer_event.hpp"
#include "sched/gpu_concurrent_sched.hpp"

#define TEMPLATE_DECL_ARG class native_data_t
#define TEMPLATE_DEF_ARG  native_data_t

template <TEMPLATE_DECL_ARG>
observer_event<TEMPLATE_DEF_ARG>::observer_event(
    ccl_gpu_sched* parent,
    native::ccl_device::device_memory_ptr<native_data_t> memory_producer,
    native::ccl_device::device_memory_ptr<int> bytes_producer,
    /*event_ptr_t device_event,*/
    size_t wait_bytes_count)
        : sched_entry(/*TODO*/ reinterpret_cast<ccl_sched*>(parent), false),
          bytes_consumed(),
          bytes_expected(wait_bytes_count),
          ready_bytes(bytes_producer),
          ready_memory_chunk(memory_producer)
//producer_event(device_event)
{
    numa_host_data.reserve(bytes_expected);
}

template <TEMPLATE_DECL_ARG>
void observer_event<TEMPLATE_DEF_ARG>::start() {
    //update_fields();
    update_status(ATL_STATUS_SUCCESS);
    if (consume_data()) {
        status = ccl_sched_entry_status_complete;
    }
}

template <TEMPLATE_DECL_ARG>
void observer_event<TEMPLATE_DEF_ARG>::update() {
    if (consume_data()) {
        status = ccl_sched_entry_status_complete;
    }
}

template <TEMPLATE_DECL_ARG>
const char* observer_event<TEMPLATE_DEF_ARG>::name() const {
    return class_name();
}

template <TEMPLATE_DECL_ARG>
bool observer_event<TEMPLATE_DEF_ARG>::consume_data() {
    //read flag
    size_t old_consumed = bytes_consumed;
    int total_produced = *ready_bytes->get();
    //int total_produced = *producer_event->get()->produced_bytes;

    if (total_produced - old_consumed) {
        //fence
        LOG_TRACE(class_name(),
                  ": ",
                  static_cast<void*>(this),
                  " - bytes produced: ",
                  total_produced,
                  ", previously bytes consumed: ",
                  old_consumed);
        std::atomic_thread_fence(std::memory_order::memory_order_seq_cst);

        //read data
        element_t* produced_chunk_ptr = ready_memory_chunk->get() + old_consumed;
        std::copy(produced_chunk_ptr,
                  produced_chunk_ptr + total_produced - old_consumed,
                  std::back_inserter(numa_host_data));
        //check finalize
        bytes_consumed = total_produced;
    }

    if (bytes_consumed >= bytes_expected) {
        CCL_ASSERT(bytes_consumed == bytes_expected,
                   "Unexpected consumed: ",
                   bytes_consumed,
                   ", expected: ",
                   bytes_expected);
        return true;
    }
    return false;
}

#undef TEMPLATE_DECL_ARG
#undef TEMPLATE_DEF_ARG
