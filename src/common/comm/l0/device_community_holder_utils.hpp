#pragma once

#include "common/comm/l0/device_community_holder.hpp"

namespace native
{

/**
 *
 * Declarations
 *
 */
namespace details
{
/**
 * class for pretty topology printing
 */
template<ccl::device_group_split_type group_id>
struct device_community_container_print_helper
{
    device_community_container_print_helper(std::ostream& out);

    template<ccl::device_topology_type class_id>
    void operator() (const device_community_container<class_id>&
                                                topology_container);

    // 'ring' requires overloading
    void operator() (const device_community_container<ccl::device_topology_type::ring>&                                            topology_container);
private:
    std::ostream& output;
};
}



/**
 *
 * Definitions
 *
 */
namespace details
{

/**
 * class for pretty topology printing definition
 */

template<ccl::device_group_split_type group_id>
device_community_container_print_helper<group_id>::device_community_container_print_helper(
                                        std::ostream& out) :
        output(out)
{
}

template<ccl::device_group_split_type group_id>
template<ccl::device_topology_type class_id>
void
device_community_container_print_helper<group_id>::operator() (
                                        const device_community_container<class_id>& topology_container)
{
    output << ::to_string(class_id) << "\n\t"
           << topology_container.storage->template to_string<group_id>();
}

template<ccl::device_group_split_type group_id>
void
device_community_container_print_helper<group_id>::operator() (
                                        const device_community_container<
                                                        ccl::device_topology_type::ring>&
                                                                        topology_container)
{
    output << ::to_string(ccl::device_topology_type::ring) << "\n\t, closed rings: "
           << topology_container.closed_rings.size() << std::endl;
    for (size_t i = 0; i < topology_container.closed_rings.size(); i++)
    {
        output << "\t\t" << topology_container.closed_rings[i]-> template to_string<group_id>();
    }

    output << "\n\t, torn-apart rings: " << topology_container.torn_apart_rings.size() << std::endl;
    for (size_t i = 0; i < topology_container.torn_apart_rings.size(); i++)
    {
        output << "\t\t" << topology_container.torn_apart_rings[i]->template to_string<group_id>();
    }
}
}
}
