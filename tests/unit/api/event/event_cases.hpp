#pragma once

//API headers with declaration of new API object
#define private public
#define protected public
#include "ccl_types.hpp"
#include "ccl_aliases.hpp"

#include "ccl_type_traits.hpp"
#include "ccl_types_policy.hpp"
#include "ccl_event_attr_ids.hpp"
#include "ccl_event_attr_ids_traits.hpp"
#include "ccl_event.hpp"

#include "environment.hpp"
#include "native_device_api/export_api.hpp"

namespace stream_suite
{

TEST(event_api, event_from_native_event_creation)
{
    std::shared_ptr<native::ccl_device::device_event> nev;
    auto ev = ccl::create_event(nev);

    ASSERT_TRUE(ev.get<ccl::event_attr_id::version>().full != nullptr);
    auto assigned_handle = ev.get<ccl::event_attr_id::native_handle>();
    ASSERT_EQ(assigned_handle->get(), nev->get());
}

TEST(event_api, event_from_native_device_context_creation)
{
    ze_event_handle_t h;
    std::shared_ptr<native::ccl_context> ctx;
    auto ev = ccl::create_event_from_attr(h, ctx);

    ASSERT_TRUE(ev.get<ccl::event_attr_id::version>().full != nullptr);
}

}
#undef protected
#undef private
