#pragma once

#include "sched/sched.hpp"

#include <list>

#define MLSL_SCHED_QUEUE_MAX_BINS (64)

struct mlsl_sched_queue_bin
{
    mlsl_sched_queue *queue = nullptr;  //!< pointer to the queue which owns the bin
    atl_comm_t *comm_ctx = nullptr;     //!< ATL communication context
    std::list<mlsl_sched*> elems{};     //!< list of schedules
    size_t priority {};                 //!< current priority
};

class mlsl_sched_queue
{
public:
    mlsl_sched_queue() = delete;
    mlsl_sched_queue(size_t capacity, atl_comm_t **comm_ctxs);
    ~mlsl_sched_queue();

    void add(mlsl_sched* sched, size_t priority);

    //todo: this method might be removed in scope of
    /**
     * Remove @b sched by its value from the list of schedules stored in @b bin
     * @note: this method will remove all scheds with identical value (address of the sched in that case)
     * @param bin holds a list of @ref mlsl_sched
     * @param sched entry in a list to be removed
     */
    void erase(mlsl_sched_queue_bin* bin, mlsl_sched* sched);

    /**
     * Remove @b it from the list of schedules stored in @b bin and returns iterator to the next schedule
     * @param bin holds a list of @ref mlsl_sched
     * @param it an iterator that points to the particular schedule in @b bin->elems
     * @return iterator to the next sched in @b bin->elems
     */
    std::list<mlsl_sched*>::iterator erase(mlsl_sched_queue_bin* bin, std::list<mlsl_sched*>::iterator it);

    /**
     * Retrieve a pointer to the bin with the highest priority and number of its elements
     * @param count[out] number of elements in bin. May have a zero if the queue has no bins with elements
     * @return a pointer to the bin with the highest priority or nullptr if there is no bins with content
     */
    mlsl_sched_queue_bin* peek(size_t& count);

    /**
     * A vector of size @ref max_bins, holds bins arranged by max_priority % max_bins
     * If @ref max_bins is less than @ref max_priority then some bins may contain schedules of mixed priorities,
     * effective bin priority will be the maximum from all variants.
     * For example, if max_priority=10, max_bins=4 then bin at idx=0 will handle schedules with priorities 0, 4 and 8.
     */
    std::vector<mlsl_sched_queue_bin> bins;

private:

    /**
     * Updates @ref max_priority when some bin is completed. Iterates through the bins starting from the previously
     * estimated @ref max_prioirity, finds the next non empty bin and saves its priority in @ref max_priority
     */
    void update_priority_on_erase();

    size_t used_bins {};       //!< Number of non empty bins
    size_t max_priority {};    //!< Maximum priority
    const size_t max_bins {};  //!< Maximum number of bins

    mlsl_fastlock_t lock;
};
