#pragma once

#define private public
#define protected public
#include "utils/utils_test.hpp"

#include "graph_resolver/device_group_resolver.hpp"
#include "graph_resolver/thread_group_resolver.hpp"
#include "group_topology/device_group_test.hpp"
#include "group_topology/thread_group_test.hpp"

#include "group_topology/process_creator_utils.hpp"
#include "group_topology/process_group_test.hpp"

#include "group_topology/cluster_group_test.hpp"
#undef protected
#undef private
