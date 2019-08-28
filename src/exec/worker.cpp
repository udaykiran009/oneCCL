#include "common/env/env.hpp"
#include "common/global/global.hpp"
#include "exec/worker.hpp"
#include "exec.hpp"

#define CCL_WORKER_CHECK_CANCEL_ITERS (32768)
#define CCL_WORKER_CHECK_UPDATE_ITERS (16384)

static void* ccl_worker_func(void* args);

ccl_worker::ccl_worker(ccl_executor* executor,
                       size_t idx,
                       std::unique_ptr<ccl_sched_queue> queue)
    : ccl_thread(idx, ccl_worker_func), should_lock(false), is_locked(false),
      executor(executor), data_queue(std::move(queue))
{ }

void ccl_worker::add(ccl_sched* sched)
{
    LOG_DEBUG("add sched ", sched, ", type ", ccl_coll_type_to_str(sched->coll_param.ctype));
    data_queue->add(sched);
}

ccl_status_t ccl_worker::do_work(size_t& processed_count)
{
    processed_count = 0;
    ccl_sched_bin* bin = data_queue->peek();
    if (bin)
    {
        return ccl_bin_progress(bin, processed_count);
    }

    return ccl_status_success;
}

void ccl_worker::clear_data_queue()
{
    data_queue->clear();
}

static void* ccl_worker_func(void* args)
{
    auto worker = static_cast<ccl_worker*>(args);
    LOG_DEBUG("worker_idx ", worker->get_idx());

    size_t iter_count = 0;
    size_t processed_count = 0;
    size_t spin_count = env_data.spin_count;
    size_t max_spin_count = env_data.spin_count;
    ccl_status_t ret;

    global_data.is_worker_thread = true;

    do
    {
        try
        {
            ret = worker->do_work(processed_count);
            if (global_data.is_ft_support &&
                unlikely(ret == ccl_status_blocked_due_to_resize || iter_count % CCL_WORKER_CHECK_UPDATE_ITERS == 0))
            {
                if (worker->should_lock.load(std::memory_order_acquire))
                {
                    worker->clear_data_queue();
                    worker->is_locked = true;
                    while (worker->should_lock.load(std::memory_order_relaxed))
                    {
                        ccl_yield(env_data.yield_type);
                    }
                    worker->is_locked = false;
                }
            }
        }
        catch (ccl::ccl_error& ccl_e)
        {
            CCL_FATAL("worker ", worker->get_idx(), " caught internal exception: ", ccl_e.what());
        }
        catch (std::exception& e)
        {
            CCL_FATAL("worker ", worker->get_idx(), " caught exception: ", e.what());
        }
        catch (...)
        {
            CCL_FATAL("worker ", worker->get_idx(), " caught general exception");
        }

        iter_count++;
        if ((iter_count % CCL_WORKER_CHECK_CANCEL_ITERS) == 0)
        {
            pthread_testcancel();
        }

        if (processed_count == 0)
        {
            spin_count--;
            if (!spin_count)
            {
                ccl_yield(env_data.yield_type);
                spin_count = 1;
            }
        }
        else
        {
            spin_count = max_spin_count;
        }
    } while (true);

    return nullptr;
}
