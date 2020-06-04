#include "exec/exec.hpp"
#include "exec/thread/listener.hpp"

void* ccl_update_comm_world_info(void* args);

ccl_listener::ccl_listener()
    : ccl_base_thread(0, ccl_update_comm_world_info)
{}

void* ccl_update_comm_world_info(void* args)
{
    ccl_listener* listener = static_cast<ccl_listener*>(args);

    int res = 0;
    listener->started = true;

    ccl::global_data& global_data = ccl::global_data::get();

    while (true)
    {
        /*
         * atl_wait_notification return values:
         * 0 - got notification, should to do some updates
         * 1 - finished by timeout, should to check if thread need to stop, in another case should to recall this func
         * TODO: change nums to enum
         * */
        res = atl_wait_notification(global_data.executor->get_atl_ctx());

        if (res == 1)
        {
            if (listener->should_stop.load(std::memory_order_acquire))
                break;
            else
                continue;
        }
        global_data.executor->is_locked = true;
        ccl_executor::worker_guard guard = global_data.executor->get_worker_lock();

        global_data.reset_resize_dependent_objects();
        atl_update(global_data.executor->get_atl_ctx());
        global_data.init_resize_dependent_objects();

        global_data.executor->update_workers();
        global_data.executor->is_locked = false;
    }

    listener->started = false;

    return nullptr;
}
