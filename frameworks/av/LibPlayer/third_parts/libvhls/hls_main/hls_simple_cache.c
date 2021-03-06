/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


//#define LOG_NDEBUG 0
#define LOG_TAG "SimpleCache"

#include "hls_simple_cache.h"
#include "hls_fifo.h"
#include "hls_utils.h"

#ifdef HAVE_ANDROID_OS
#include "hls_common.h"
#else
#include "hls_debug.h"
#endif
#include <amthreadpool.h>

typedef struct _SimpleCache {
    int size;
    int free_size;
    HLSFifoBuffer* fifo;
    pthread_mutex_t lock;
    unsigned long read_thread_id;
} SimpleCache_t;

int hls_simple_cache_alloc(int size_max, void** handle)
{
    if (size_max <= 0) {
        LOGE("cache size must greater than zero\n");
        return -1;
    }
    SimpleCache_t* cache = (SimpleCache_t*)malloc(sizeof(SimpleCache_t));
    if (cache) {
        cache->read_thread_id = 0;
        cache->fifo = hls_fifo_alloc(size_max);
        if (cache->fifo == NULL) {
            free(cache);
            return -1;
        }

        cache->size = size_max;
        cache->free_size = 0;
        pthread_mutex_init(&cache->lock, NULL);
        *handle = cache;

        LOGV("Create simple cache,size:%d.%d KB\n", size_max / 1024, size_max % 1024);
        return 0;
    }
    return -1;
}
int hls_simple_cache_write(void* handle, void* buf, int size) //just judge free space by caller
{
    int oldsize;
    if (handle == NULL) {
        return -1;
    }
    SimpleCache_t* cache = (SimpleCache_t*)handle;
    pthread_mutex_lock(&cache->lock);
    oldsize = hls_fifo_size(cache->fifo);
    hls_fifo_generic_write(cache->fifo, buf, size, NULL);
    cache->free_size = hls_fifo_space(cache->fifo);
    pthread_mutex_unlock(&cache->lock);
    if (oldsize < 100 && cache->read_thread_id != 0) {
        amthreadpool_thread_wake(cache->read_thread_id);
    }
    return size;
}

int hls_simple_cache_get_free_space(void* handle)
{
    if (handle == NULL) {
        return -1;
    }
    SimpleCache_t* cache = (SimpleCache_t*)handle;
    pthread_mutex_lock(&cache->lock);

    int size = hls_fifo_space(cache->fifo);

    pthread_mutex_unlock(&cache->lock);

    return size;
}
int hls_simple_cache_get_data_size(void* handle)
{
    if (handle == NULL) {
        return -1;
    }
    SimpleCache_t* cache = (SimpleCache_t*)handle;
    pthread_mutex_lock(&cache->lock);

    int size = hls_fifo_size(cache->fifo);

    pthread_mutex_unlock(&cache->lock);

    return size;

}
int hls_simple_cache_get_cache_size(void* handle)
{
    if (handle == NULL) {
        return -1;
    }
    SimpleCache_t* cache = (SimpleCache_t*)handle;
    pthread_mutex_lock(&cache->lock);

    int size = cache->size;

    pthread_mutex_unlock(&cache->lock);

    return size;

}
int hls_simple_cache_reset(void* handle)
{
    if (handle == NULL) {
        return -1;
    }
    SimpleCache_t* cache = (SimpleCache_t*)handle;
    pthread_mutex_lock(&cache->lock);

    hls_fifo_reset(cache->fifo);
    cache->free_size = cache->size;
    pthread_mutex_unlock(&cache->lock);

    return 0;
}
int hls_simple_cache_read(void* handle, void* buffer, int size)
{
    if (handle == NULL) {
        return -1;
    }
    SimpleCache_t* cache = (SimpleCache_t*)handle;
    pthread_mutex_lock(&cache->lock);
    int rsize = HLSMIN(hls_fifo_size(cache->fifo), size);
    hls_fifo_generic_read(cache->fifo, buffer, rsize, NULL);
    cache->free_size = hls_fifo_space(cache->fifo);
    pthread_mutex_unlock(&cache->lock);

    return rsize;
}

int hls_simple_cache_block_read(void* handle, void* buffer, int size, int wait_us)
{
    int readed;
    readed = hls_simple_cache_read(handle, buffer, size);
    if (!readed) {
        SimpleCache_t* cache = (SimpleCache_t*)handle;
        cache->read_thread_id = pthread_self();
        amthreadpool_thread_usleep(wait_us);
        readed = hls_simple_cache_read(handle, buffer, size);
    }
    return readed;
}
int hls_simple_cache_free(void* handle)
{
    if (handle == NULL) {
        return -1;
    }
    SimpleCache_t* cache = (SimpleCache_t*)handle;
    pthread_mutex_lock(&cache->lock);

    hls_fifo_free(cache->fifo);
    free(cache);

    pthread_mutex_unlock(&cache->lock);

    return 0;
}
int hls_simple_cache_revert(void* handle)
{
    if (handle == NULL) {
        return -1;
    }
    SimpleCache_t* cache = (SimpleCache_t*)handle;
    pthread_mutex_lock(&cache->lock);

    hls_fifo_revert(cache->fifo);
    pthread_mutex_unlock(&cache->lock);

    return 0;
}

int hls_simple_cache_grow_space(void* handle, int size)
{
    if (handle == NULL) {
        return -1;
    }
    SimpleCache_t* cache = (SimpleCache_t*)handle;
    int ret = -1;
    pthread_mutex_lock(&cache->lock);

    ret = hls_fifo_grow(cache->fifo, size);
    pthread_mutex_unlock(&cache->lock);
    return ret;
}

int hls_simple_cache_move_to_pos(void* handle, int pos)
{
    if (handle == NULL) {
        return -1;
    }
    SimpleCache_t* cache = (SimpleCache_t*)handle;
    pthread_mutex_lock(&cache->lock);

    hls_fifo_revert(cache->fifo);
    if (pos > 0) {
        hls_fifo_drain(cache->fifo, pos);
    }
    pthread_mutex_unlock(&cache->lock);

    return 0;
}