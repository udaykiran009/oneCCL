#pragma once

#include "sched/sched.hpp"

#define MLSL_SCHED_QUEUE_MAX_BINS (64)

struct mlsl_sched_queue_bin
{
    mlsl_sched_queue *queue = nullptr;
    atl_comm_t *comm_ctx = nullptr;
    mlsl_sched *elems = nullptr;
    size_t priority {};
    size_t elem_count {};
};

class mlsl_sched_queue
{
public:
    mlsl_sched_queue() = delete;
    mlsl_sched_queue(size_t capacity, atl_comm_t **comm_ctxs);
    ~mlsl_sched_queue();

    void add(mlsl_sched* sched, size_t priority);
    void erase(mlsl_sched_queue_bin* bin, mlsl_sched* sched);
    mlsl_sched_queue_bin* peek(size_t& count);

    std::vector<mlsl_sched_queue_bin> bins;

private:
    size_t used_bins {};
    size_t max_bins {};
    size_t max_priority {};

    mlsl_fastlock_t lock;
};
