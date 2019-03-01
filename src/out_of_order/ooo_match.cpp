#include "common/global/global.hpp"
#include "out_of_order/ooo_match.hpp"
#include "sched/entry_factory.hpp"

out_of_order::ooo_match::ooo_match(mlsl_executor& exec, comm_id_storage& comm_ids)
    : m_executor(exec), m_comm_ids(comm_ids)
{
    MLSL_LOG(INFO, "Configuring out-of-order collectives support");

    auto id = std::unique_ptr<comm_id>(new comm_id(m_comm_ids));
    m_service_comm = std::make_shared<mlsl_comm>(m_executor.proc_idx, m_executor.proc_count, std::move(id));

    if (m_executor.proc_idx != 0)
    {
        mlsl_sched* service_sched = build_bcast_sched();

        MLSL_LOG(DEBUG, "Submit persistent service schedule(s) %p", service_sched);
        m_executor.start(service_sched);
    }
}

void out_of_order::ooo_match::postpone_for_tensor(const std::string& tensor_name, mlsl_sched* sched)
{
    //1. Check if there is unresolved tensor name
    auto unresolved = m_unresolved_comms.find(tensor_name);
    if(unresolved != m_unresolved_comms.end())
    {
        MLSL_LOG(DEBUG, "Found postponed comm creation for tensor %s, id %hu", tensor_name.c_str(),
                 unresolved->second->value());
        //1.2. create new communicator with reserved comm_id
        auto tensor_comm = sched->coll_param.comm->clone_with_new_id(std::move(unresolved->second));

        m_tensor_comm_storage.add_comm_for_tensor(tensor_name, tensor_comm);
        sched->coll_param.comm = tensor_comm.get();
        for (size_t part_idx = 0; part_idx < sched->partial_scheds.size(); ++part_idx)
        {
            sched->partial_scheds[part_idx]->coll_param.comm = tensor_comm.get();
        }
        sched->start(&m_executor);

        m_unresolved_comms.erase(unresolved);
    }
    else
    {
        //2. postpone this schedule
        MLSL_LOG(INFO, "Postpone sched %d %p for tensor %s", sched->coll_param.ctype, sched, tensor_name.c_str());
        m_postponed_schedules.postpone_for_tensor(tensor_name, sched);
    }
}

void out_of_order::ooo_match::create_comm_and_run_sched(const std::string& tensor_name)
{
    auto id = std::unique_ptr<comm_id>(new comm_id(m_comm_ids));

    //get user provided communicator from the postponed schedule
    auto original_comm = m_postponed_schedules.get_sched_comm_by_tensor(tensor_name);
    if(!original_comm)
    {
        //root ranks has broadcasted tensor name, but other rank has not yet received request
        //from the user, so we can't create a communicator. Need to wait for request from the user.
        MLSL_LOG(DEBUG, "Can't find postponed sched for tenosr %s, postpone comm creation, reserved id %hu",
                 tensor_name.c_str(), id->value());
        m_unresolved_comms.emplace(tensor_name, std::move(id));
        return;
    }

    MLSL_LOG(DEBUG, "create comm id %hu for tensor %s", id->value(), tensor_name.c_str());

    auto tensor_comm = original_comm->clone_with_new_id(std::move(id));

    //store new communicator
    m_tensor_comm_storage.add_comm_for_tensor(tensor_name, tensor_comm);
    //run all postponed schedules
    m_postponed_schedules.run_scheds_for_tensor(tensor_name, tensor_comm.get(), &m_executor);
}

mlsl_sched* out_of_order::ooo_match::build_bcast_sched(const char* tensor_name)
{
    MLSL_ASSERT_FMT(m_service_comm->rank() != 0 || tensor_name != nullptr, "Only root rank can pass a valid tensor name");

    bool temp_sched = tensor_name != nullptr;
    mlsl_coll_param bcast_param{};
    bcast_param.ctype = temp_sched ? mlsl_coll_service_temporal : mlsl_coll_service_persistent;
    bcast_param.dtype = mlsl_dtype_internal_char;
    bcast_param.comm = m_service_comm.get();
    auto bcast_sched = new mlsl_sched(bcast_param);

    MLSL_LOG(DEBUG, "Building service sched %p, req %p", bcast_sched, bcast_sched->req);

    /* TODO: remove work with partial schedule, use original bcast_sched for execution */
    bcast_sched->partial_scheds.emplace_back(std::make_shared<mlsl_sched>(bcast_param));
    bcast_sched->partial_scheds[0]->coll_attr.priority = bcast_sched->coll_attr.priority;
    bcast_sched->partial_scheds[0]->coll_param.comm = m_service_comm.get();
    bcast_sched->partial_scheds[0]->root = bcast_sched;
    bcast_sched->partial_scheds[0]->set_request(bcast_sched->req);

    if(tensor_name != nullptr)
    {
        MLSL_LOG(DEBUG, "Copy tensor %s to sched", tensor_name);
        strncpy(bcast_sched->partial_scheds[0]->coll_attr.match_id, tensor_name, MLSL_MATCH_ID_MAX_LEN);
    }

    mlsl_coll_build_bcast(bcast_sched->partial_scheds[0].get(),
                          bcast_sched->partial_scheds[0]->coll_attr.match_id,
                          MLSL_MATCH_ID_MAX_LEN,
                          mlsl_dtype_internal_char,
                          0);

    bcast_sched->partial_scheds[0]->add_barrier();

    //created tensor_comm entry will have an address of bcast_sched->match_id where tensor name will be broadcasted
    entry_factory::make_tensor_comm_entry(bcast_sched->partial_scheds[0].get(), this,
                                          bcast_sched->partial_scheds[0]->coll_attr.match_id);

    //overwrite schedule type that was set in mlsl_coll_build_bcast
    bcast_sched->partial_scheds[0]->coll_param.ctype = bcast_sched->coll_param.ctype;

    bcast_sched->prepare_partial_scheds();

    return bcast_sched;
}
