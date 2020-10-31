#pragma once

//API headers with declaration of new API object
#define private public
#define protected public
#include "common/event/impls/empty_event.hpp"
#include "common/event/impls/native_event.hpp"
#include "common/event/impls/event_impl.hpp"

#undef protected
#undef private

namespace event_suite {

TEST(event, event_empty_creation) {
    ccl::v1::event ev;
    ASSERT_TRUE(ev.get_impl());
    ASSERT_TRUE(ev.test() == true);
    ASSERT_TRUE(ev.cancel() == true);
    if (ev) { // operator bool()
        ASSERT_TRUE(true);
    }
    else {
        // never should be
        ASSERT_TRUE(false);
    }
}

TEST(event, move_event) {
    /* move constructor test */
    ccl::v1::event orig_ev;

    auto orig_inner_impl_ptr = orig_ev.get_impl().get();
    auto moved_ev = (std::move(orig_ev));
    auto moved_inner_impl_ptr = moved_ev.get_impl().get();

    ASSERT_EQ(orig_inner_impl_ptr, moved_inner_impl_ptr);
    ASSERT_TRUE(!orig_ev.get_impl());

    /* move assignment test */
    ccl::v1::event orig_ev2;

    ccl::v1::event moved_ev2;
    moved_ev2 = std::move(orig_ev2);

    ASSERT_TRUE(orig_ev2.get_impl().get() == nullptr);
    ASSERT_TRUE(moved_ev2.get_impl().get() != orig_ev2.get_impl().get());

    /* event(impl_value_t&& impl) constructor test */
    ccl::v1::event orig_ev3;
    ccl::v1::event moved_ev3(std::move(orig_ev3.get_impl()));

    ASSERT_TRUE(orig_ev3.get_impl().get() == nullptr);
    ASSERT_TRUE(moved_ev3.get_impl().get() != orig_ev3.get_impl().get());
}

TEST(event, native_event) {
    #ifndef CCL_ENABLE_SYCL
        ccl::v1::event::native_t native_ev; // this is shared_ptr for all, exclude sycl
        ccl::v1::event new_ev = ccl::v1::event::create_from_native(native_ev);
        ccl::v1::event::native_t new_native_ev = new_ev.get_native();
        (void)new_native_ev;
    #endif
}

} // namespace event_suite
