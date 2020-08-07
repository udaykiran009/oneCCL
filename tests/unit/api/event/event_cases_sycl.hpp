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


namespace stream_suite
{

TEST(event_api, event_from_sycl_event_creation)
{
    auto ev = cl::sycl::event();
    auto str = ccl::create_event(ev);

    ASSERT_TRUE(str.get<ccl::event_attr_id::version>().full != nullptr);
}

TEST(event_api, event_from_sycl_device_context_creation)
{
    auto ctx = cl::sycl::context();
    auto ev = cl::sycl::event();
    cl_event h = ev.get();
    auto str = ccl::create_event_from_attr(h, ctx);

    ASSERT_TRUE(str.get<ccl::event_attr_id::version>().full != nullptr);
}

}
#undef protected
#undef private
