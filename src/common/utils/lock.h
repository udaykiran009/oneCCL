/*
 * Copyright (c) 2013-2014 Intel Corporation. All rights reserved.
 * Copyright (c) 2016 Cisco Systems, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef LOCK_H
#define LOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#if PT_LOCK_SPIN == 1

#define mlsl_fastlock_t_ pthread_spinlock_t
#define mlsl_fastlock_init_(lock) pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE)
#define mlsl_fastlock_destroy_(lock) pthread_spin_destroy(lock)
#define mlsl_fastlock_acquire_(lock) pthread_spin_lock(lock)
#define mlsl_fastlock_tryacquire_(lock) pthread_spin_trylock(lock)
#define mlsl_fastlock_release_(lock) pthread_spin_unlock(lock)

#else

#define mlsl_fastlock_t_ pthread_mutex_t
#define mlsl_fastlock_init_(lock) pthread_mutex_init(lock, NULL)
#define mlsl_fastlock_destroy_(lock) pthread_mutex_destroy(lock)
#define mlsl_fastlock_acquire_(lock) pthread_mutex_lock(lock)
#define mlsl_fastlock_tryacquire_(lock) pthread_mutex_trylock(lock)
#define mlsl_fastlock_release_(lock) pthread_mutex_unlock(lock)

#endif /* PT_LOCK_SPIN */

#if ENABLE_DEBUG

typedef struct {
    mlsl_fastlock_t_ impl;
    int is_initialized;
} mlsl_fastlock_t;

static inline int mlsl_fastlock_init(mlsl_fastlock_t *lock)
{
    int ret;

    ret = mlsl_fastlock_init_(&lock->impl);
    lock->is_initialized = !ret;
    return ret;
}

static inline void mlsl_fastlock_destroy(mlsl_fastlock_t *lock)
{
    int ret;

    assert(lock->is_initialized);
    lock->is_initialized = 0;
    ret = mlsl_fastlock_destroy_(&lock->impl);
    assert(!ret);
}

static inline void mlsl_fastlock_acquire(mlsl_fastlock_t *lock)
{
    int ret;

    assert(lock->is_initialized);
    ret = mlsl_fastlock_acquire_(&lock->impl);
    assert(!ret);
}

static inline int mlsl_fastlock_tryacquire(mlsl_fastlock_t *lock)
{
    assert(lock->is_initialized);
    return mlsl_fastlock_tryacquire_(&lock->impl);
}

static inline void mlsl_fastlock_release(mlsl_fastlock_t *lock)
{
    int ret;

    assert(lock->is_initialized);
    ret = mlsl_fastlock_release_(&lock->impl);
    assert(!ret);
}

#else /* !ENABLE_DEBUG */

#  define mlsl_fastlock_t mlsl_fastlock_t_
#  define mlsl_fastlock_init(lock) mlsl_fastlock_init_(lock)
#  define mlsl_fastlock_destroy(lock) mlsl_fastlock_destroy_(lock)
#  define mlsl_fastlock_acquire(lock) mlsl_fastlock_acquire_(lock)
#  define mlsl_fastlock_tryacquire(lock) mlsl_fastlock_tryacquire_(lock)
#  define mlsl_fastlock_release(lock) mlsl_fastlock_release_(lock)

#endif

#ifdef __cplusplus
}
#endif

#endif /* LOCK_H */
