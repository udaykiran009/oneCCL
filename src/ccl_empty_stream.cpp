#include "ccl_types.hpp"
#include "ccl_aliases.hpp"
#include "ccl_types_policy.hpp"
#include "ccl_type_traits.hpp"

#include "ccl_stream_attr_ids.hpp"
#include "ccl_stream_attr_ids_traits.hpp"
#include "ccl_stream.hpp"

// Core file with PIMPL implementation
//#include "stream_impl.hpp"

namespace ccl
{

template<class attr>
CCL_API attr ccl_empty_attr::create_empty()
{
    return attr{ccl_empty_attr::version};
}

CCL_API stream default_stream = ccl_empty_attr::create_empty<stream>();

}
