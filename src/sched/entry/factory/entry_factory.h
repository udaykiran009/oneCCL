#pragma once

#include "sched/entry/entry.hpp"
#include "sched/sched.hpp"

#include <functional>
#include <list>
#include <memory>

// declares interface for all entries creations
namespace entry_factory {
template <class EntryType, class... Arguments>
EntryType* create(ccl_sched* sched, Arguments&&... args);

namespace detail {
template <class EntryType>
struct entry_creator {
    template <class T, class... U>
    friend T* create(ccl_sched* sched, U&&... args);

    template <class T, ccl_sched_add_mode mode, class... U>
    friend T* create(ccl_sched* sched, U&&... args);

    template <ccl_sched_add_mode mode, class... Arguments>
    static EntryType* make_entry(ccl_sched* sched, Arguments&&... args) {
        return static_cast<EntryType*>(sched->add_entry(
            std::unique_ptr<EntryType>(new EntryType(sched, std::forward<Arguments>(args)...)),
            ccl_sched_base::add_entry_mode_t<mode>()));
    }
};
} // namespace detail
} // namespace entry_factory
