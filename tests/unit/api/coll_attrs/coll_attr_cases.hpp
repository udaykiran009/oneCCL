#pragma once

//API headers with declaration of new API object
#include "ccl.hpp"
#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"
#include "ccl_coll_attr.hpp"

// Core file with PIMPL implementation
#include "coll/coll_attributes.hpp"

namespace coll_attr_suite
{
TEST(coll_attr, creation)
{
    ccl::allgatherv_attr_t attr = ccl::create_coll_attr<allgatherv_attr_t>();
    (void)attr;
}

}
