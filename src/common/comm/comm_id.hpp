#pragma once

#include "common/comm/comm_id_storage.hpp"

class comm_id
{
public:
    comm_id() = delete;
    explicit comm_id(comm_id_storage& storage) : m_storage(storage), m_id(m_storage.acquire_id())
    {}

    ~comm_id()
    {
        m_storage.release_id(m_id);
    }

    mlsl_comm_id_t value() const
    {
        return m_id;
    }

private:
    comm_id_storage& m_storage;
    mlsl_comm_id_t m_id;
};