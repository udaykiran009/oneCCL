#pragma once

template <class visitor_to_connect>
struct base_connector_interface
{
    using visitor = visitor_to_connect;

    virtual ~base_connector_interface() noexcept = default;
    virtual bool operator() (visitor_to_connect& to_connect) = 0;
};
