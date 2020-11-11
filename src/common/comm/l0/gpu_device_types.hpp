#pragma once

#include "common/comm/l0/device_types.hpp"
#include "common/comm/l0/device_types_fwd.hpp"
#include "common/comm/l0/device_containers.hpp"
#include "common/comm/l0/device_containers_utils.hpp"

namespace native {

using specific_device_storage = device_storage_t<SUPPORTED_DEVICES_DECL_LIST>;
using specific_plain_device_storage = plain_device_storage<SUPPORTED_DEVICES_DECL_LIST>;
using specific_indexed_device_storage = indexed_device_storage<SUPPORTED_DEVICES_DECL_LIST>;
using specific_device_variant_t = device_variant_t<SUPPORTED_DEVICES_DECL_LIST>;

} // namespace native
