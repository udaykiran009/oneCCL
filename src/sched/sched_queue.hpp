#pragma once

#include "sched/sched.hpp"

#include <list>

#define MLSL_SCHED_QUEUE_MAX_BINS (64)

struct mlsl_sched_queue_bin
{
    mlsl_sched_queue *queue = nullptr;
    atl_comm_t *comm_ctx = nullptr;
    std::list<mlsl_sched*> elems{};
    size_t priority {};
};

class mlsl_sched_queue
{
public:
    mlsl_sched_queue() = delete;
    mlsl_sched_queue(size_t capacity, atl_comm_t **comm_ctxs);
    ~mlsl_sched_queue();

    void add(mlsl_sched* sched, size_t priority);
    void erase(mlsl_sched_queue_bin* bin, mlsl_sched* sched);
    std::list<mlsl_sched*>::iterator erase(mlsl_sched_queue_bin* bin, std::list<mlsl_sched*>::iterator it);
    mlsl_sched_queue_bin* peek(size_t& count);

    std::vector<mlsl_sched_queue_bin> bins;

private:

    void update_priority_on_erase();
    size_t used_bins {};
    size_t max_bins {};
    size_t max_priority {};

    mlsl_fastlock_t lock;
};
