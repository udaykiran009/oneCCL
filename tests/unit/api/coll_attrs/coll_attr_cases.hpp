#pragma once

//API headers with declaration of new API object
#define private public
#define protected public
#include "ccl_types.h"
#include "ccl_aliases.hpp"
#include "ccl_types_policy.hpp"
#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"
#include "ccl_coll_attr.hpp"

// Core file with PIMPL implementation
#include "coll_attr_impl.hpp"
#include "coll/coll_attributes.hpp"

#include <array>
#include "allgather_cases.hpp"
#include "allreduce_cases.hpp"
#include "barrier_cases.hpp"
#undef protected
#undef private
