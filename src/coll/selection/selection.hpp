#pragma once

#include "coll/selection/selector_wrapper.hpp"

bool ccl_is_direct_algo(const ccl_selector_param& param);
bool ccl_is_device_side_algo(const ccl_selector_param& param);

bool ccl_can_use_topo_a2a_algo(const ccl_selector_param& param);
bool ccl_can_use_topo_ring_algo(const ccl_selector_param& param);

bool ccl_can_use_datatype(ccl_coll_algo algo, const ccl_selector_param& param);
