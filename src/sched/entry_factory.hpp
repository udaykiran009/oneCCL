#pragma once

#include "entry_factory.h"

#include "sched/sched.hpp"
#include "sched/entry_factory.hpp"
#include "sched/sync_object.hpp"
#include "sched/entry/send_entry.hpp"
#include "sched/entry/recv_entry.hpp"
#include "sched/entry/write_entry.hpp"
#include "sched/entry/reduce_local_entry.hpp"
#include "sched/entry/recv_reduce_entry.hpp"
#include "sched/entry/copy_entry.hpp"
#include "sched/entry/sync_entry.hpp"
#include "sched/entry/prologue_entry.hpp"
#include "sched/entry/epilogue_entry.hpp"
#include "sched/entry/wait_value_entry.hpp"
#include "sched/entry/function_entry.hpp"
#include "sched/entry/probe_entry.hpp"
#include "sched/entry/register_entry.hpp"
#include "sched/entry/deregister_entry.hpp"
#include "sched/entry/chain_call_entry.hpp"
#include "sched/entry/nop_entry.hpp"
#include "sched/entry/coll/coll_entry.hpp"
#include "sched/entry/coll/allgatherv_entry.hpp"
#include "sched/entry/coll/allreduce_entry.hpp"
#include "sched/entry/coll/barrier_entry.hpp"
#include "sched/entry/coll/bcast_entry.hpp"
#include "sched/entry/coll/reduce_entry.hpp"

namespace entry_factory
{
    // generic interface for entry creation
    template<class EntryType, class ...Arguments>
    EntryType* make_entry(ccl_sched* sched, Arguments &&...args)
    {
        LOG_DEBUG("creating ", EntryType::entry_class_name(), " entry");
        return detail::entry_creator<EntryType>::create(sched, std::forward<Arguments>(args)...);
    }


    /* Example for non-standard entry 'my_non_standard_entry' creation
    namespace detail
    {
        template <>
        class entry_creator<my_non_standard_entry>
        {
            public:
            static my_non_standard_entry* create(/ *** specific parameters for construction *** /)
            {
                auto &&new_entry = std::unique_ptr<my_non_standard_entry>(
                            new my_non_standard_entry(/ *** specific parameters for construction *** /));

                //Add custom contruction/registration logic, if needed

                return static_cast<my_non_standard_entry*>(sched->add_entry(std::move(new_entry)));
            }
        };
    }*/
}
