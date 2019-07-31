#pragma once

#include "sched/entry/entry.hpp"
#include "coll/coll.hpp"

#include <memory>
#include <list>
#include <functional>


//declares interface for all entries creations
namespace entry_factory
{
    template<class EntryType, class ...Arguments>
    EntryType* make_entry(ccl_sched* sched, Arguments &&...args);


    namespace detail
    {
        template<class EntryType>
        struct entry_creator
        {
            template <class T, class ...U>
            friend T* make_entry(ccl_sched* sched, U &&...args);

            template<class ...Arguments>
            static EntryType* create(ccl_sched* sched, Arguments &&...args)
            {
                return static_cast<EntryType*>(sched->add_entry(std::unique_ptr<EntryType> (
                                                                        new EntryType(sched,
                                                                        std::forward<Arguments>(args)...))));
            }
        };
    }
}
