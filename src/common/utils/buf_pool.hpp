/*
 * Copyright (c) 2016 Intel Corporation. All rights reserved.
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

#ifndef BUF_POOL_H
#define BUF_POOL_H

#include "common/utils/utils.hpp"
#include "common/log/log.hpp"
#include "iccl_types.h"

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

struct iccl_buf_pool;
typedef iccl_status_t (*iccl_buf_region_alloc_hndlr)(void *pool_ctx, void *addr,
                                                     size_t len, void **context);
typedef void (*iccl_buf_region_free_hndlr)(void *pool_ctx, void *context);

struct iccl_buf_attr {
    size_t                      size;
    size_t                      alignment;
    size_t                      max_cnt;
    size_t                      chunk_cnt;
    iccl_buf_region_alloc_hndlr alloc_hndlr;
    iccl_buf_region_free_hndlr  free_hndlr;
    void                        *ctx;
    uint8_t                     track_used;
};

struct iccl_buf_pool {
    size_t               entry_sz;
    size_t               num_allocated;
    iccl_slist_t         buf_list;
    iccl_slist_t         region_list;
    struct iccl_buf_attr attr;
};

struct iccl_buf_region {
    iccl_slist_entry_t entry;
    char               *mem_region;
    void               *context;
#if ENABLE_DEBUG
    size_t             num_used;
#endif
};

struct iccl_buf_footer {
    struct iccl_buf_region *region;
};

union iccl_buf {
    iccl_slist_entry_t entry;
    uint8_t data[0];
};

iccl_status_t iccl_buf_pool_create_attr(struct iccl_buf_attr *attr,
                                        struct iccl_buf_pool **buf_pool);

/* create buffer pool with alloc/free handlers */
iccl_status_t iccl_buf_pool_create_ex(struct iccl_buf_pool **pool,
                                      size_t size, size_t alignment,
                                      size_t max_cnt, size_t chunk_cnt,
                                      iccl_buf_region_alloc_hndlr alloc_hndlr,
                                      iccl_buf_region_free_hndlr free_hndlr,
                                      void *pool_ctx);

/* create buffer pool */
static inline iccl_status_t iccl_buf_pool_create(struct iccl_buf_pool **pool,
                                                 size_t size, size_t alignment,
                                                 size_t max_cnt, size_t chunk_cnt)
{
    return iccl_buf_pool_create_ex(pool, size, alignment,
                                   max_cnt, chunk_cnt,
                                   NULL, NULL, NULL);
}

static inline iccl_status_t iccl_buf_avail(struct iccl_buf_pool *pool)
{
	return !ICCL_SLIST_EMPTY(&pool->buf_list);
}

iccl_status_t iccl_buf_grow(struct iccl_buf_pool *pool);

#if ENABLE_DEBUG

void *iccl_buf_get(struct iccl_buf_pool *pool);
void iccl_buf_release(struct iccl_buf_pool *pool, void *buf);

#else

static inline void *iccl_buf_get(struct iccl_buf_pool *pool)
{
    iccl_slist_entry_t *entry;
    entry = ICCL_SLIST_REMOVE_HEAD(&pool->buf_list);
    return entry;
}

static inline void iccl_buf_release(struct iccl_buf_pool *pool, void *buf)
{
    union iccl_buf *iccl_buf = buf;
    ICCL_SLIST_INSERT_HEAD(&pool->buf_list, iccl_buf);
}
#endif

static inline void *iccl_buf_get_ex(struct iccl_buf_pool *pool, void **context)
{
    union iccl_buf *buf;
    struct iccl_buf_footer *buf_ftr;

    buf = iccl_buf_get(pool);
    buf_ftr = (struct iccl_buf_footer *)((char *)buf + pool->attr.size);
    ICCL_ASSERT(context);
    *context = buf_ftr->region->context;
    return buf;
}

static inline void *iccl_buf_alloc(struct iccl_buf_pool *pool)
{
    if (unlikely(!iccl_buf_avail(pool))) {
        if (iccl_buf_grow(pool))
	    return NULL;
    }
    return iccl_buf_get(pool);
}

static inline void *iccl_buf_alloc_ex(struct iccl_buf_pool *pool, void **context)
{
    union iccl_buf *buf;
    struct iccl_buf_footer *buf_ftr;

    buf = iccl_buf_alloc(pool);
    if (unlikely(!buf))
        return NULL;

    buf_ftr = (struct iccl_buf_footer *)((char *) buf + pool->attr.size);
    ICCL_ASSERT(context);
    *context = buf_ftr->region->context;
    return buf;
}

#if ENABLE_DEBUG
static inline int iccl_buf_use_ftr(struct iccl_buf_pool *pool)
{
    return 1;
}
#else
static inline int iccl_buf_use_ftr(struct iccl_buf_pool *pool)
{
    return (pool->attr.alloc_hndlr || pool->attr.free_hndlr) ? 1 : 0;
}
#endif

static inline void *iccl_buf_get_ctx(struct iccl_buf_pool *pool, void *buf)
{
    struct iccl_buf_footer *buf_ftr;
    ICCL_ASSERT(iccl_buf_use_ftr(pool));
    buf_ftr = (struct iccl_buf_footer *)((char *) buf + pool->attr.size);
    return buf_ftr->region->context;
}

void iccl_buf_pool_destroy(struct iccl_buf_pool *pool);

#endif /* BUF_POOL_H */
