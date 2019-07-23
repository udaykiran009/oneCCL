#include "sched/entry/entry.hpp"
#include "common/log/log.hpp"

void postponed_fields::add_available(ccl_sched_entry_field_id id)
{
    available_fields.emplace(id);
}

void postponed_fields::add(ccl_sched_entry_field_id id,
                           ccl_sched_entry_field_function_t fn,
                           const void* ctx,
                           bool update_once)
{
    CCL_ASSERT(available_fields.find(id) != available_fields.end(),
                    "unexpected field_id %d", id);
    CCL_ASSERT(fields.find(id) == fields.end(),
                    "duplicated field_id %d", id);
    fields.emplace(id, postponed_field(id, fn, ctx, update_once));
}

void postponed_fields::update()
{
    std::map<ccl_sched_entry_field_id, postponed_field>::iterator it;
    for (it = fields.begin(); it != fields.end();)
    {
        it->second.fn(it->second.ctx, entry->get_field_ptr(it->second.id));
        if (it->second.update_once)
            it = fields.erase(it);
        else
            it++;
    }
}
