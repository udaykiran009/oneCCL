#pragma once

#include "common/comm/comm_id_storage.hpp"

class comm_id
{
public:
    comm_id() = delete;
    explicit comm_id(comm_id_storage& storage,
                     bool internal = false) : id_storage(storage), id(id_storage.acquire_id(internal))
    {}

    comm_id(comm_id_storage& storage,
            mlsl_comm_id_t preallocated_id) : id_storage(storage), id(preallocated_id)
    {}

    ~comm_id()
    {
        id_storage.release_id(id);
    }

    mlsl_comm_id_t value() const
    {
        return id;
    }

private:
    comm_id_storage& id_storage;
    mlsl_comm_id_t id;
};
