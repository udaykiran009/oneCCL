#include "out_of_order/ooo_match.hpp"
#include "sched/entry_factory.hpp"

out_of_order::ooo_match::ooo_match(mlsl_executor* exec)
{
    MLSL_LOG(INFO, "Configuring out-of-order collectives support");
    MLSL_ASSERTP_FMT(exec, "Provided invalid executor");

    mlsl_comm* service_comm_ptr;
    mlsl_comm_create_internal(exec->proc_idx, exec->proc_count,
                              &service_comm_ptr, rank_to_global_rank_map{});

    service_comm = std::shared_ptr<mlsl_comm>(service_comm_ptr, mlsl_comm_free);

    if (exec->proc_idx != 0)
    {
        mlsl_sched* service_sched = build_bcast_sched();

        MLSL_LOG(DEBUG, "Submit persistent service schedule(s) %p", service_sched);
        exec->start_sched(service_sched);
    }
}

void out_of_order::ooo_match::postpone_for_tensor(const std::string& tensor_name, mlsl_sched* sched)
{
    //1. Check if there is unresolved tensor name
    auto unresolved = unresolved_comm.find(tensor_name);
    if(unresolved != unresolved_comm.end())
    {
        MLSL_LOG(INFO, " Found postponed comm creation for tensor %s", tensor_name.c_str());
        //1.2. create new communicator with reserved comm_id
        mlsl_comm* tensor_comm = nullptr;
        mlsl_status_t result = mlsl_comm_create_copy(sched->coll_param.comm, &tensor_comm, unresolved->second);
        MLSL_ASSERT_FMT(result == mlsl_status_success, "Failed to create tensor comm");

        tensor_comm_storage.add_comm_for_tensor(tensor_name, tensor_comm);
        mlsl_request* req;
        sched->coll_param.comm = tensor_comm;
        for (size_t part_idx = 0; part_idx < sched->partial_sched_count; ++part_idx)
        {
            sched->partial_scheds[part_idx]->coll_param.comm = tensor_comm;
        }
        mlsl_sched_start(sched, &req);
    }
    else
    {
        //2. postpone this schedule
        MLSL_LOG(INFO, "Postpone sched %d %p for tensor %s", sched->coll_param.ctype, sched, tensor_name.c_str());
        postponed_schedules.postpone_for_tensor(tensor_name, sched);
    }
}

void out_of_order::ooo_match::create_comm_and_run_sched(const std::string& tensor_name)
{
    mlsl_comm_id_t comm_id{};
    mlsl_status_t result = mlsl_comm_id_acquire(&comm_id);
    MLSL_ASSERT_FMT(result == mlsl_status_success, "Failed to allocate new comm id");

    //get user provided communicator from the postponed schedule
    auto original_comm = postponed_schedules.get_sched_comm_by_tensor(tensor_name);
    if(!original_comm)
    {
        //root ranks has broadcasted tensor name, but other rank has not yet received request
        //from the user, so we can't create a communicator. Need to wait for request from the user.
        MLSL_LOG(INFO, "Can't find postponed sched for tenosr %s, postpone comm creation", tensor_name.c_str());
        unresolved_comm.emplace(tensor_name, comm_id);
        return;
    }

    //create a copy of the original communicator (comm id will be different)
    mlsl_comm* tensor_comm = nullptr;
    result = mlsl_comm_create_copy(original_comm, &tensor_comm, comm_id);
    MLSL_ASSERT_FMT(result == mlsl_status_success, "Failed to create tenosr comm");

    //store new communicator
    tensor_comm_storage.add_comm_for_tensor(tensor_name, tensor_comm);
    //run all postponed schedules
    postponed_schedules.run_scheds_for_tensor(tensor_name, tensor_comm);
}

mlsl_sched* out_of_order::ooo_match::build_bcast_sched(const char* tensor_name)
{
    MLSL_ASSERT_FMT(service_comm->rank != 0 || tensor_name != nullptr, "Only root rank can pass a valid tensor name");
    mlsl_sched* bcast_sched;
    mlsl_sched_tensor_bcast(service_comm.get(), &bcast_sched, tensor_name != nullptr);
    mlsl_request_create(&bcast_sched->req);

    MLSL_LOG(DEBUG, "Building service sched %p, req %p", bcast_sched, bcast_sched->req);

    mlsl_get_priority_range(nullptr, &bcast_sched->coll_attr.priority);
    bcast_sched->coll_param.comm = service_comm.get();
    bcast_sched->partial_sched_count = 1;
    bcast_sched->partial_scheds = static_cast<mlsl_sched**>(MLSL_MALLOC(sizeof(mlsl_sched*), "scheds"));

    bcast_sched->partial_scheds[0] = new mlsl_sched{};

    bcast_sched->partial_scheds[0]->coll_attr.priority = bcast_sched->coll_attr.priority;
    bcast_sched->partial_scheds[0]->coll_param.comm = service_comm.get();
    bcast_sched->partial_scheds[0]->root = bcast_sched;

    if(tensor_name != nullptr)
    {
        MLSL_LOG(DEBUG, "Copy tensor %s to sched", tensor_name);
        strncpy(bcast_sched->partial_scheds[0]->coll_attr.match_id, tensor_name, MLSL_MATCH_ID_MAX_LEN);
    }

    mlsl_coll_build_bcast(bcast_sched->partial_scheds[0],
                          bcast_sched->partial_scheds[0]->coll_attr.match_id,
                          MLSL_MATCH_ID_MAX_LEN,
                          mlsl_dtype_internal_char,
                          0);

    mlsl_sched_add_barrier(bcast_sched->partial_scheds[0]);

    //created tensor_comm entry will have an address of bcast_sched->match_id where tensor name will be broadcasted
    bcast_sched->partial_scheds[0]->add_entry(entry_factory::make_tensor_comm_entry(this,
                                                           bcast_sched->partial_scheds[0]->coll_attr.match_id));

    //overwrite schedule type that was set in mlsl_coll_build_bcast
    bcast_sched->partial_scheds[0]->coll_param.ctype = bcast_sched->coll_param.ctype;

    return bcast_sched;
}
