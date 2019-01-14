#include "common/comm/comm.hpp"
#include "sched/sched.hpp"
#include "coll/coll_algorithms.hpp"

#include <vector>

//todo: move mapping to communicator class
class mlsl_comm_map
{
public:
    using comm_to_rank_map = std::unordered_map<mlsl_comm*, rank_to_global_rank_map>;

    mlsl_status_t add_comm(mlsl_comm* comm, rank_to_global_rank_map&& ranks_map);
    mlsl_status_t add_comm_copy(mlsl_comm* original, mlsl_comm* new_comm);
    mlsl_status_t remove_comm(mlsl_comm* comm);
    mlsl_status_t get_global_rank(mlsl_comm* comm, size_t rank, size_t& global_rank);
private:
    comm_to_rank_map comm_map;
};

mlsl_status_t mlsl_comm_map::add_comm(mlsl_comm* comm, rank_to_global_rank_map&& ranks_map)
{
    if (comm_map.find(comm) != comm_map.end())
    {
        MLSL_LOG(INFO, "Communicator %p is already in the map", comm);
        return mlsl_status_invalid_arguments;
    }

    MLSL_LOG(DEBUG, "Add comm %p with %zu ranks to the map", comm, ranks_map.size());
    comm_map.emplace(comm, ranks_map);
    return mlsl_status_success;
}

mlsl_status_t mlsl_comm_map::add_comm_copy(mlsl_comm* original, mlsl_comm* new_comm)
{
    if (comm_map.find(new_comm) != comm_map.end())
    {
        MLSL_LOG(INFO, "Communicator %p is already in the map", new_comm);
        return mlsl_status_invalid_arguments;
    }

    if (comm_map.find(original) == comm_map.end())
    {
        MLSL_LOG(INFO, "Original communicator %p was not found in the map", original);
        return mlsl_status_invalid_arguments;
    }

    auto ranks_map = comm_map[original];
    MLSL_LOG(DEBUG, "Add comm %p with %zu ranks to the map", new_comm, ranks_map.size());
    comm_map.insert(std::make_pair(new_comm, ranks_map));
    return mlsl_status_success;
}

mlsl_status_t mlsl_comm_map::remove_comm(mlsl_comm* comm)
{
    if (comm_map.find(comm) == comm_map.end())
    {
        MLSL_LOG(INFO, "No communicator %p was found in the map", comm);
        return mlsl_status_invalid_arguments;
    }

    comm_map.erase(comm);
    return mlsl_status_success;
}

mlsl_status_t mlsl_comm_map::get_global_rank(mlsl_comm* comm, size_t rank,
                                                 size_t& global_rank)
{
    if (comm_map.find(comm) == comm_map.end())
    {
        //no communicator in the map
        MLSL_LOG(ERROR, "No comm %p was found", comm);
        return mlsl_status_invalid_arguments;
    }

    auto ranks_map = comm_map[comm];
    if (ranks_map.empty())
    {
        //global comm and its copies do not have entries in the map
        MLSL_LOG(DEBUG, "Direct mapping of rank %zu", rank);
        global_rank = rank;
        return mlsl_status_success;
    }

    if (ranks_map.find(rank) == ranks_map.end())
    {
        MLSL_LOG(ERROR, "No rank %zu was found in comm %p", rank, comm);
        return mlsl_status_invalid_arguments;
    }

    global_rank = ranks_map[rank];
    MLSL_LOG(DEBUG, "Comm %p, map rank %zu to global %zu", comm, rank, global_rank);

    return mlsl_status_success;
}

static mlsl_comm_map comm_map;

mlsl_comm* global_comm = nullptr;

static mlsl_status_t mlsl_comm_create_with_color(mlsl_comm** comm, int color);

static mlsl_status_t mlsl_comm_exchange_colors(std::vector<int>& colors);

mlsl_status_t mlsl_comm_create(mlsl_comm** comm, mlsl_comm_attr_t* comm_attr)
{
    MLSL_ASSERT(comm);

    if (!comm_attr)
    {
        MLSL_LOG(DEBUG, "Duplicating global comm");
        mlsl_status_t result = mlsl_comm_create_internal(global_data.comm->rank,
                                                         global_data.comm->size,
                                                         comm,
                                                         rank_to_global_rank_map { });
        return result;
    }

    return mlsl_comm_create_with_color(comm, comm_attr->color);
}

mlsl_status_t mlsl_comm_get_global_rank(mlsl_comm* comm, size_t rank, size_t* global_rank)
{
    MLSL_ASSERT(global_rank);
    return comm_map.get_global_rank(comm, rank, *global_rank);
}

mlsl_status_t mlsl_comm_create_internal(size_t rank, size_t size, mlsl_comm **comm,
                                               rank_to_global_rank_map&& ranks_map)
{
    mlsl_comm_id_t comm_id;
    mlsl_status_t result = mlsl_comm_id_acquire(&comm_id);
    if (result != mlsl_status_success)
    {
        return result;
    }

    mlsl_comm *c = static_cast<mlsl_comm*>(MLSL_CALLOC(sizeof(mlsl_comm), "comm"));

    c->comm_id = comm_id;
    c->next_sched_id = 0;
    c->rank = rank;
    c->size = size;
    c->pof2 = mlsl_pof2(size);
    *comm = c;

    result = comm_map.add_comm(c, std::move(ranks_map));

    MLSL_LOG(DEBUG, "New comm: rank %zu, size %zu, tag %hu", rank, size,
             comm_id);

    return result;
}

mlsl_status_t mlsl_comm_create_copy(mlsl_comm* original, mlsl_comm** new_comm, mlsl_comm_id_t comm_id)
{
    mlsl_comm *c = new mlsl_comm{};
    memcpy(c, original, sizeof(mlsl_comm));
    c->comm_id = comm_id;
    c->next_sched_id = 0;
    *new_comm = c;

    mlsl_status_t  result = comm_map.add_comm_copy(original, c);

    MLSL_LOG(DEBUG, "New comm: rank %zu, size %zu, tag %hu", c->rank, c->size, comm_id);

    return result;
}

static mlsl_status_t mlsl_comm_create_with_color(mlsl_comm** comm, int color)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_comm* global_comm = global_data.comm;
    std::vector<int> all_colors(global_comm->size);
    all_colors[global_comm->rank] = color;

    status = mlsl_comm_exchange_colors(all_colors);
    if (status != mlsl_status_success)
    {
        return status;
    }

    size_t colors_match = 0;
    size_t new_rank = 0;

    rank_to_global_rank_map ranks_map;
    for (size_t i = 0; i < global_comm->size; ++i)
    {
        if (all_colors[i] == color)
        {
            MLSL_LOG(DEBUG, "map local rank %zu to global %zu", colors_match, i);
            ranks_map[colors_match] = i;
            ++colors_match;
            if (i < global_comm->rank)
            {
                ++new_rank;
            }
        }
    }
    if (colors_match == 0)
    {
        MLSL_LOG(ERROR, "No colors matched to %d, seems to be exchange issue", color);
        return mlsl_status_invalid_arguments;
    }
    if(colors_match == global_comm->size)
    {
        //Exact copy of the global communicator, use empty map
        ranks_map.clear();
    }

    MLSL_CALL(mlsl_comm_create_internal(new_rank, colors_match, comm, std::move(ranks_map)));
    if (status != mlsl_status_success)
    {
        return status;
    }

    MLSL_LOG(DEBUG, "New comm: color %d, rank %zu, size %zu, comm_id %hu", color,
             (*comm)->rank, (*comm)->size, (*comm)->comm_id);

    return status;
}

mlsl_status_t mlsl_comm_free(mlsl_comm *comm)
{
    MLSL_ASSERT(comm);
    MLSL_LOG(DEBUG, "Free comm %p", comm);
    mlsl_status_t result = mlsl_comm_id_release(comm->comm_id);
    if (result == mlsl_status_success)
    {
        result = comm_map.remove_comm(comm);
    }
    MLSL_FREE(comm);
    return result;
}

static mlsl_status_t mlsl_comm_exchange_colors(std::vector<int>& colors)
{
    const size_t exchange_count = 1;
    std::vector<size_t> recv_counts(colors.size(), exchange_count);
    mlsl_coll_attr_t coll_attr{};
    coll_attr.to_cache = false;
    mlsl_request_t request;

    mlsl_status_t status;
    MLSL_CALL(mlsl_allgatherv(colors.data(), exchange_count,
                              colors.data(), recv_counts.data(),
                              mlsl_dtype_int, &coll_attr,
                              nullptr, &request));

    //wait for completion
    MLSL_CALL(mlsl_wait(request));

    return status;
}
