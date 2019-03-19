#include "sched/entry_types/entry.hpp"
#include "common/log/log.hpp"

void postponed_fields::add_available(mlsl_sched_entry_field_id id)
{
    available_fields.emplace(id);
}

void postponed_fields::add(mlsl_sched_entry_field_id id,
                           mlsl_sched_entry_field_function_t fn,
                           const void* ctx,
                           bool update_once)
{
    MLSL_ASSERT_FMT(available_fields.find(id) != available_fields.end(),
                    "unexpected field_id %d", id);
    MLSL_ASSERT_FMT(fields.find(id) == fields.end(),
                    "duplicated field_id %d", id);
    fields.emplace(id, postponed_field(id, fn, ctx, update_once));
}

void postponed_fields::update()
{
    std::map<mlsl_sched_entry_field_id, postponed_field>::iterator it;
    for (it = fields.begin(); it != fields.end();)
    {
        it->second.fn(it->second.ctx, entry->get_field_ptr(it->second.id));
        if (it->second.update_once)
            it = fields.erase(it);
        else
            it++;
    }
}
