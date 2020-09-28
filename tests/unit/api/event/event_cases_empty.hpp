#pragma once

//API headers with declaration of new API object
#define private public
#define protected public
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "common/event_internal/ccl_event/ccl_event_attr_ids.hpp"
#include "common/event_internal/ccl_event/ccl_event_attr_ids_traits.hpp"
#include "common/event_internal/ccl_event/ccl_event.hpp"

#include "environment.hpp"
#include "oneapi/ccl/native_device_api/export_api.hpp"

namespace stream_suite {


} // namespace stream_suite
#undef protected
#undef private
