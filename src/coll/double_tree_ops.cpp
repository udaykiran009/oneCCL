#include "sched/sched.hpp"
#include "sched/entry_factory.hpp"
#include "common/utils/tree.hpp"

static void bcast_tree(const bin_tree& tree,
                       ccl_sched* sched,
                       ccl_buffer buffer,
                       size_t count,
                       ccl_datatype_internal_t dtype,
                       ccl_op_id_t op_id)
{
    if (tree.parent() != -1)
    {
        LOG_DEBUG("recv from parent ", tree.parent());
        entry_factory::make_recv_entry(sched, buffer, count, dtype, static_cast<size_t>(tree.parent()), op_id);
        sched->add_barrier();
    }
    if (tree.left() != -1)
    {
        LOG_DEBUG("send to left ", tree.left());
        entry_factory::make_send_entry(sched, buffer, count, dtype, static_cast<size_t>(tree.left()), op_id);
    }
    if (tree.right() != -1)
    {
        LOG_DEBUG("send to right ", tree.right());
        entry_factory::make_send_entry(sched, buffer, count, dtype, static_cast<size_t>(tree.right()), op_id);
    }
}

static void reduce_tree(const bin_tree& tree,
                        ccl_sched* sched,
                        ccl_buffer buffer,
                        size_t count,
                        ccl_datatype_internal_t dtype,
                        ccl_reduction_t reduction,
                        ccl_op_id_t op_id)
{
    if (tree.left() != -1)
    {
        LOG_DEBUG("recv_reduce left ", tree.left());
        entry_factory::make_recv_reduce_entry(sched, buffer, count, nullptr, dtype, reduction,
                                              static_cast<size_t>(tree.left()), ccl_buffer(), op_id);
    }
    if (tree.right() != -1)
    {
        LOG_DEBUG("recv_reduce right ", tree.right());
        entry_factory::make_recv_reduce_entry(sched, buffer, count, nullptr, dtype, reduction,
                                              static_cast<size_t>(tree.right()), ccl_buffer(), op_id);
    }
    if (tree.parent() != -1)
    {
        if (tree.left() != -1 || tree.right() != -1)
        {
            sched->add_barrier();
        }
        LOG_DEBUG("send to parent ", tree.parent());
        entry_factory::make_send_entry(sched, buffer, count, dtype, static_cast<size_t>(tree.parent()), op_id);
    }
}

static void reduce_bcast_tree(const bin_tree& tree,
                              ccl_sched* sched,
                              ccl_buffer buffer,
                              size_t count,
                              ccl_datatype_internal_t dtype,
                              ccl_reduction_t reduction,
                              ccl_op_id_t op_id)
{
    if (tree.left() != -1)
    {
        LOG_DEBUG("recv_reduce left ", tree.left());
        entry_factory::make_recv_reduce_entry(sched, buffer, count, nullptr, dtype, reduction,
                                              static_cast<size_t>(tree.left()), ccl_buffer(), op_id);
    }
    if (tree.right() != -1)
    {
        LOG_DEBUG("recv_reduce right ", tree.right());
        entry_factory::make_recv_reduce_entry(sched, buffer, count, nullptr, dtype, reduction,
                                              static_cast<size_t>(tree.right()), ccl_buffer(), op_id);
    }
    if (tree.parent() != -1)
    {
        if (tree.left() != -1 || tree.right() != -1)
        {
            sched->add_barrier();
        }

        LOG_DEBUG("send to parent ", tree.parent());
        entry_factory::make_send_entry(sched, buffer, count, dtype, static_cast<size_t>(tree.parent()), op_id);

        LOG_DEBUG("recv from parent ", tree.parent());
        entry_factory::make_recv_entry(sched, buffer, count, dtype, static_cast<size_t>(tree.parent()), op_id);
    }

    if (tree.left() != -1 || tree.right() != -1)
    {
        sched->add_barrier();
    }

    if (tree.left() != -1)
    {
        LOG_DEBUG("send to left ", tree.left());
        entry_factory::make_send_entry(sched, buffer, count, dtype, static_cast<size_t>(tree.left()), op_id);
    }
    if (tree.right() != -1)
    {
        LOG_DEBUG("send to right ", tree.right());
        entry_factory::make_send_entry(sched, buffer, count, dtype, static_cast<size_t>(tree.right()), op_id);
    }
}

ccl_status_t ccl_coll_build_double_tree_op(ccl_sched* sched,
                                           ccl_coll_type coll_type,
                                           ccl_buffer send_buf,
                                           ccl_buffer recv_buf,
                                           size_t count,
                                           ccl_datatype_internal_t dtype,
                                           ccl_reduction_t op,
                                           const double_tree& dtree)
{
    ccl_status_t status = ccl_status_success;

    LOG_DEBUG("build double tree ", ccl_coll_type_to_str(coll_type));

    if (coll_type != ccl_coll_bcast && send_buf != recv_buf)
    {
        LOG_DEBUG("out of place op");
        entry_factory::make_copy_entry(sched, send_buf, recv_buf, count, dtype);
        sched->add_barrier();
    }

    size_t t1_count = count / 2;
    size_t t2_count = count - t1_count;

    ccl_buffer t1_start = recv_buf;
    ccl_buffer t1_end = t1_start + t1_count * dtype->size;

    ccl_buffer t2_start = t1_end;
    ccl_buffer t2_end = t2_start + t2_count * dtype->size;

    //todo: evaluate/configure k param;
    size_t parts = 1;

    size_t t1_part_count = t1_count / parts;
    size_t t2_part_count = t2_count / parts;

    LOG_DEBUG("t1: start ", t1_start,
              ", end ", t1_end,
              ", count ", t1_count,
              ", part size ", t1_part_count);

    LOG_DEBUG("t2: start ", t2_start,
              ", end ", t2_end,
              ", count ", t2_count,
              ", part size ", t2_part_count);

    ccl_op_id_t t1_op_id = 0;
    ccl_op_id_t t2_op_id = t1_op_id + static_cast<ccl_op_id_t>(parts);

    for (size_t iter = 0; iter < parts; ++iter)
    {
        size_t t1_work_count = t1_part_count;
        if (t1_start + t1_work_count * iter > t1_end)
        {
            LOG_DEBUG("iter ", iter, ", t1 size ", t1_work_count, " exceeds ", t1_end, ", align to ",
                      t1_end - (t1_start + t1_work_count * iter));
            t1_work_count = t1_end - (t1_start + t1_work_count * iter);
        }

        ccl_buffer t1_work_buf = t1_start + t1_work_count * dtype->size * iter;

        size_t t2_work_count = t2_part_count;
        if (t2_start + t2_work_count * iter > t2_end)
        {
            LOG_DEBUG("iter ", iter, ", t2 size ", t2_work_count, " exceeds ", t2_end, ", align to ",
                      t2_end - (t2_start + t2_work_count * iter));
            t2_work_count = t2_end - (t2_start + t2_work_count * iter);
        }

        ccl_buffer t2_work_buf = t2_start + t2_work_count * dtype->size * iter;

        std::function<void(ccl_sched*)> funcT1;
        std::function<void(ccl_sched*)> funcT2;

        const auto& t1 = dtree.T1();
        const auto& t2 = dtree.T2();

        switch (coll_type)
        {
            case ccl_coll_bcast:
                entry_factory::make_chain_call_entry(sched,
                    [t1_work_buf, t1_work_count, dtype, t1_op_id, t1](ccl_sched* s)
                    {
                        bcast_tree(t1, s, t1_work_buf, t1_work_count, dtype, t1_op_id);
                    }, "bcast_t1");

                entry_factory::make_chain_call_entry(sched,
                    [t2_work_buf, t2_work_count, dtype, t2_op_id, t2](ccl_sched* s)
                    {
                        bcast_tree(t2, s, t2_work_buf, t2_work_count, dtype, t2_op_id);
                    }, "bcast_t2");

                break;
            case ccl_coll_reduce:
            {
                if(sched->coll_param.comm->rank() % 2 == 0)
                {
                    //even ranks are leaves in T2, start schedule with T2
                    entry_factory::make_chain_call_entry(sched,
                        [t2_work_buf, t2_work_count, dtype, t2_op_id, op, t2](ccl_sched* s)
                        {
                            reduce_tree(t2, s, t2_work_buf, t2_work_count, dtype, op, t2_op_id);
                        },"reduce_t2");

                    entry_factory::make_chain_call_entry(sched,
                        [t1_work_buf, t1_work_count, dtype, t1_op_id, op, t1](ccl_sched* s)
                        {
                            reduce_tree(t1, s, t1_work_buf, t1_work_count, dtype, op, t1_op_id);
                        },"reduce_t1");
                }
                else
                {
                    entry_factory::make_chain_call_entry(sched,
                        [t2_work_buf, t2_work_count, dtype, t2_op_id, op, t2](ccl_sched* s)
                        {
                            reduce_tree(t2, s, t2_work_buf, t2_work_count, dtype, op, t2_op_id);
                        },"reduce_t2");

                    entry_factory::make_chain_call_entry(sched,
                        [t1_work_buf, t1_work_count, dtype, t1_op_id, op, t1](ccl_sched* s)
                        {
                            reduce_tree(t1, s, t1_work_buf, t1_work_count, dtype, op, t1_op_id);
                        },"reduce_t1");
                }

                break;
            }
            case ccl_coll_allreduce:
            {
                if(sched->coll_param.comm->rank() % 2 == 0)
                {
                    //even ranks are leaves in T2, start schedule with T2
                    entry_factory::make_chain_call_entry(sched,
                        [t2_work_buf, t2_work_count, dtype, t2_op_id, op, t2](ccl_sched* s)
                        {
                            reduce_bcast_tree(t2, s, t2_work_buf, t2_work_count, dtype, op, t2_op_id);
                        }, "reduce_bcast_t2");

                    entry_factory::make_chain_call_entry(sched,
                        [t1_work_buf, t1_work_count, dtype, t1_op_id, op, t1](ccl_sched* s)
                        {
                            reduce_bcast_tree(t1, s, t1_work_buf, t1_work_count, dtype, op, t1_op_id);
                        },
                        "reduce_bcast_t1");
                }
                else
                {
                    entry_factory::make_chain_call_entry(sched,
                        [t1_work_buf, t1_work_count, dtype, t1_op_id, op, t1](ccl_sched* s)
                        {
                            reduce_bcast_tree(t1, s, t1_work_buf, t1_work_count, dtype, op, t1_op_id);
                        },
                        "reduce_bcast_t1");

                    entry_factory::make_chain_call_entry(sched,
                        [t2_work_buf, t2_work_count, dtype, t2_op_id, op, t2](ccl_sched* s)
                        {
                            reduce_bcast_tree(t2, s, t2_work_buf, t2_work_count, dtype, op, t2_op_id);
                        }, "reduce_bcast_t2");
                }
                break;
            }
            default:
                CCL_FATAL("unsupported double tree op ", ccl_coll_type_to_str(coll_type));
        }

        ++t1_op_id;
        ++t2_op_id;
    }

    sched->add_barrier();

    return status;
}
