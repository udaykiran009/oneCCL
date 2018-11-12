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

#include "common/utils/buf_pool.h"

static inline void mlsl_buf_set_region(union mlsl_buf *buf,
                                       struct mlsl_buf_region *region,
                                       struct mlsl_buf_pool *pool)
{
    struct mlsl_buf_footer *buf_ftr;
    if (mlsl_buf_use_ftr(pool)) {
        buf_ftr = (struct mlsl_buf_footer *) ((char *) buf + pool->attr.size);
	buf_ftr->region = region;
    }
}

mlsl_status_t mlsl_buf_grow(struct mlsl_buf_pool *pool)
{
    mlsl_status_t ret;
    size_t i;
    union mlsl_buf *buf;
    struct mlsl_buf_region *buf_region;

    if (pool->attr.max_cnt && pool->num_allocated >= pool->attr.max_cnt) {
        return mlsl_status_invalid_arguments;
    }

    buf_region = MLSL_CALLOC(sizeof(*buf_region), "buf_region");
    if (!buf_region)
        return mlsl_status_out_of_resource;

    buf_region->mem_region = MLSL_MEMALIGN(pool->attr.alignment,
                                           pool->attr.chunk_cnt * pool->entry_sz,
                                           "buf_region->mem_region");
    if (!buf_region->mem_region) {
        ret = mlsl_status_out_of_resource;
        goto err_buf_reg;
    }

    if (pool->attr.alloc_hndlr) {
        ret = pool->attr.alloc_hndlr(pool->attr.ctx, buf_region->mem_region,
                                     pool->attr.chunk_cnt * pool->entry_sz,
                                     &buf_region->context);
	if (ret)
            goto err_alloc;
    }

    for (i = 0; i < pool->attr.chunk_cnt; i++) {
        buf = (union mlsl_buf *)(buf_region->mem_region + i * pool->entry_sz);
	mlsl_buf_set_region(buf, buf_region, pool);
        MLSL_SLIST_INSERT_TAIL(&pool->buf_list, buf);
    }

    MLSL_SLIST_INSERT_TAIL(&pool->region_list, buf_region);
    pool->num_allocated += pool->attr.chunk_cnt;
    return mlsl_status_success;
err_alloc:
    MLSL_FREE(buf_region->mem_region);
err_buf_reg:
    MLSL_FREE(buf_region);
    return ret;
}

mlsl_status_t mlsl_buf_pool_create_attr(struct mlsl_buf_attr *attr,
                                        struct mlsl_buf_pool **buf_pool)
{
    size_t entry_sz;

    (*buf_pool) = MLSL_CALLOC(sizeof(**buf_pool), "buf_pool");
    if (!*buf_pool)
        return mlsl_status_out_of_resource;

    (*buf_pool)->attr = *attr;

    entry_sz = mlsl_buf_use_ftr(*buf_pool) ?
               (attr->size + sizeof(struct mlsl_buf_footer)) : attr->size;
    (*buf_pool)->entry_sz = mlsl_aligned_sz(entry_sz, attr->alignment);

    MLSL_SLIST_INIT(&(*buf_pool)->buf_list);
    MLSL_SLIST_INIT(&(*buf_pool)->region_list);

    if (mlsl_buf_grow(*buf_pool)) {
        MLSL_FREE(*buf_pool);
        return mlsl_status_out_of_resource;
    }
    return mlsl_status_success;
}

mlsl_status_t mlsl_buf_pool_create_ex(struct mlsl_buf_pool **buf_pool,
                                      size_t size, size_t alignment,
                                      size_t max_cnt, size_t chunk_cnt,
                                      mlsl_buf_region_alloc_hndlr alloc_hndlr,
                                      mlsl_buf_region_free_hndlr free_hndlr,
                                      void *pool_ctx)
{
    struct mlsl_buf_attr attr = {
        .size        = size,
        .alignment   = alignment,
        .max_cnt     = max_cnt,
        .chunk_cnt   = chunk_cnt,
        .alloc_hndlr = alloc_hndlr,
        .free_hndlr  = free_hndlr,
        .ctx         = pool_ctx,
        .track_used  = 1,
    };
    return mlsl_buf_pool_create_attr(&attr, buf_pool);
}

#if ENABLE_DEBUG
void *mlsl_buf_get(struct mlsl_buf_pool *pool)
{
    struct slist_entry *entry;
    struct mlsl_buf_footer *buf_ftr;

    entry = MLSL_SLIST_REMOVE_HEAD(&pool->buf_list);
    buf_ftr = (struct mlsl_buf_footer *) ((char *) entry + pool->attr.size);
    buf_ftr->region->num_used++;
    MLSL_ASSERT_FMT(buf_ftr->region->num_used);
    return entry;
}

void mlsl_buf_release(struct mlsl_buf_pool *pool, void *buf)
{
    union mlsl_buf *mlsl_buf = buf;
    struct mlsl_buf_footer *buf_ftr;

    buf_ftr = (struct mlsl_buf_footer *)((char *)buf + pool->attr.size);
    MLSL_ASSERT_FMT(buf_ftr->region->num_used);
    buf_ftr->region->num_used--;
    MLSL_SLIST_INSERT_HEAD(&pool->buf_list, mlsl_buf);
}
#endif

void mlsl_buf_pool_destroy(struct mlsl_buf_pool *pool)
{
    mlsl_slist_entry_t *entry;
    struct mlsl_buf_region *buf_region;

    while (!MLSL_SLIST_EMPTY(&pool->region_list)) {
        entry = MLSL_SLIST_REMOVE_HEAD(&pool->region_list);
	buf_region = container_of(entry, struct mlsl_buf_region, entry);
#if ENABLE_DEBUG
        if (pool->attr.track_used)
            MLSL_ASSERT(buf_region->num_used == 0);
#endif
	if (pool->attr.free_hndlr)
            pool->attr.free_hndlr(pool->attr.ctx, buf_region->context);
	MLSL_FREE(buf_region->mem_region);
	MLSL_FREE(buf_region);
    }
    MLSL_FREE(pool);
}
