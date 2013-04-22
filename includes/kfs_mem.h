/* vim: set expandtab ts=4 sw=4:                               */

/*-===========================================================-*/
/*  Copyright (c) 2010   LoongStore                            */
/*  LoongStore Corporation.                                    */
/*  All rights reserved.                                       */
/*                                                             */
/*  Author:                                                    */
/*        KevinKW                                              */
/*-===========================================================-*/

/*
 * Define the memory interfaces that are used by KFS.
 *
 */
#ifndef __KFS_MEM_H__
#define __KFS_MEM_H__

/* Memory check */
#include <linux/slab.h>
#ifdef KFS_DEBUG_MEM
typedef enum {
    MEM_NET = 0,
    MEM_IO,
    MEM_WORK,
    MEM_PATH,
    MEM_FS,
    MEM_NORMAL,
    MEM_MAX
} mem_t;

struct mem_check {
    u64 allocated[MEM_MAX];
    u64 freed[MEM_MAX];
};

typedef enum {
    OBJ_FILE = 0,
    OBJ_PATH,
    OBJ_WORK,
    OBJ_PAGELOCK,
    OBJ_MAX
} object_t;

struct object_stat {
    u64 allocated[OBJ_MAX];
    u64 freed[OBJ_MAX];
};

extern int init_mem_check(void);
extern void destroy_mem_check(void);
extern void check_mem_usage(void);
extern int kfs_read_mem(char *buf, char **start, off_t offset, int count, int *eof, void *data);
extern int kfs_read_obj(char *buf, char **start, off_t offset, int count, int *eof, void *data);
#else
#define init_mem_check() 0
#define check_mem_usage()
#define destroy_mem_check()
#define kfs_alloc_obj(type)
#define kfs_free_obj(type)
#endif

#ifdef KFS_DEBUG_MEM
extern u64 kfs_percpu_mem_allocated;
extern u64 kfs_percpu_mem_freed;
extern struct mem_check __percpu *kfs_mem_usage;
extern struct mem_check __percpu *kfs_cache_usage;
extern void kfs_mem_check_alloc(struct mem_check *mc, mem_t type, size_t size);
extern void kfs_mem_check_free(struct mem_check *mc, mem_t type, size_t size);
extern void kfs_alloc_obj(object_t type);
extern void kfs_free_obj(object_t type);

static inline void *__kfs_alloc(mem_t type, size_t size, gfp_t flags, const char *f)
{
    void *p = kmalloc(size + sizeof(u64), flags);
    u64 *s;
    if (p) {
#ifdef KFS_DEBUG_MEM2
        kdebug(LOG_MEMORY, "Alloc by %s(): %p %zd\n", f, p, size);
#endif
        kfs_mem_check_alloc(kfs_mem_usage, type, size);
        s = (u64 *)p;
        *s = size;
        p = (char*)p + sizeof(u64);
    }
    return p;
}

static inline void __kfs_free(mem_t type, const void *p, const char *f)
{
    u64 *s;
    if (p) {
        p = ((char*)p) - sizeof(u64);
        s = (u64 *)p;
#ifdef KFS_DEBUG_MEM2
        kdebug(LOG_MEMORY, "Free by %s(): %p %zd\n", f, p, *s);
#endif
        kfs_mem_check_free(kfs_mem_usage, type, *s);
    }
    kfree(p);
}

#define kfs_alloc(mt, s, f) __kfs_alloc(mt, s, f, __FUNCTION__)
#define kfs_free(mt, p) __kfs_free(mt, p, __FUNCTION__)

#else
#define kfs_alloc(mt, s, f) kmalloc(s, f)
#define kfs_free(mt, p) kfree(p)
#endif

#ifdef KFS_DEBUG_MEM
static inline void *__kfs_cache_alloc(mem_t type, size_t size, KMEM_CACHE_T * cachep,
        gfp_t flags, const char *f)
{
    void *p = kmem_cache_alloc(cachep, flags);
    if (p) {
#ifdef KFS_DEBUG_MEM2
        kdebug(LOG_MEMORY, "Cache alloc by %s(): %p %zd\n", f, p, size);
#endif
        kfs_mem_check_alloc(kfs_cache_usage, type, size);
    }
    return p;
}

static inline void __kfs_cache_free(mem_t type, size_t size, KMEM_CACHE_T * cachep,
        void *p, const char *f)
{
    if (p) {
        kfs_mem_check_free(kfs_cache_usage, type, size);
#ifdef KFS_DEBUG_MEM2
        kdebug(LOG_MEMORY, "Cache free by %s(): %p %zd\n", f, p, size);
#endif
    }
    kmem_cache_free(cachep, p);
}

static inline void *__kfs_pool_alloc(mem_t type, size_t size, mempool_t * pool,
        gfp_t flags, const char *f)
{
    void *p = mempool_alloc(pool, flags);
    if (p) {
#ifdef KFS_DEBUG_MEM2
        kdebug(LOG_MEMORY, "Cache alloc by %s(): %p %zd\n", f, p, size);
#endif
        kfs_mem_check_alloc(kfs_cache_usage, type, size);
    }
    return p;
}

static inline void __kfs_pool_free(mem_t type, size_t size, mempool_t * pool,
        void *p, const char *f)
{
    if (p) {
        kfs_mem_check_free(kfs_cache_usage, type, size);
#ifdef KFS_DEBUG_MEM2
        kdebug(LOG_MEMORY, "Cache free by %s(): %p %zd\n", f, p, size);
#endif
    }
    mempool_free(p, pool);
}

#define kfs_cache_alloc(mt, type, cachep, flags) \
    (type *)__kfs_cache_alloc(mt, sizeof(type), cachep, flags, __FUNCTION__)
#define kfs_cache_free(mt, cachep, x) __kfs_cache_free(mt, sizeof(*x), cachep, x, __FUNCTION__)

#ifdef KFS_USE_MEMPOOL
#define kfs_pool_alloc(mt, type, pool, flags) \
    (type *)__kfs_pool_alloc(mt, sizeof(type), pool, flags, __FUNCTION__)
#define kfs_pool_free(mt, pool, x) __kfs_pool_free(mt, sizeof(*x), pool, x, __FUNCTION__)
#endif
#else
#define kfs_cache_alloc(mt, type, cachep, flags) \
    (type *)kmem_cache_alloc(cachep, flags)
#define kfs_cache_free(mt, cachep, p) kmem_cache_free(cachep, p)
#ifdef KFS_USE_MEMPOOL
#define kfs_pool_alloc(mt, type, pool, flags) \
    (type *)mempool_alloc(pool, flags)
#define kfs_pool_free(mt, pool, p) mempool_free(p, pool)
#endif
#endif

extern KMEM_CACHE_T *kfs_file_cachep;
extern KMEM_CACHE_T *kfs_ioreq_cachep;
extern KMEM_CACHE_T *kfs_work_cachep;
extern KMEM_CACHE_T *kfs_path_cachep;
extern KMEM_CACHE_T *kfs_range_cachep;
#ifdef KFS_USE_MEMPOOL
extern mempool_t *kfs_file_pool;
extern mempool_t *kfs_ioreq_pool;
extern mempool_t *kfs_work_pool;
extern mempool_t *kfs_path_pool;
extern mempool_t *kfs_range_pool;
#endif

#ifdef KFS_USE_MEMPOOL
#define kfs_cache_alloc_file(flag) kfs_pool_alloc(MEM_FS, struct kfs_file, kfs_file_pool, flag);
#define kfs_cache_free_file(file) kfs_pool_free(MEM_FS, kfs_file_pool, file);
#define kfs_cache_alloc_ioreq(flag) kfs_pool_alloc(MEM_IO, struct kfs_io, kfs_ioreq_pool, flag);
#define kfs_cache_free_ioreq(ioreq) kfs_pool_free(MEM_IO, kfs_ioreq_pool, ioreq);
#define kfs_cache_alloc_work(flag) kfs_pool_alloc(MEM_WORK, struct kfs_work, kfs_work_pool, flag);
#define kfs_cache_free_work(work) kfs_pool_free(MEM_WORK, kfs_work_pool, work);
#define kfs_cache_alloc_path(flag) kfs_pool_alloc(MEM_PATH, struct kfs_path, kfs_path_pool, flag);
#define kfs_cache_free_path(path) kfs_pool_free(MEM_PATH, kfs_path_pool, path);
#define kfs_cache_alloc_range(flag) kfs_pool_alloc(MEM_IO, struct kfs_file_range, kfs_range_pool, flag);
#define kfs_cache_free_range(range) kfs_pool_free(MEM_IO, kfs_range_pool, range);
#else
#define kfs_cache_alloc_file(flag) kfs_cache_alloc(MEM_FS, struct kfs_file, kfs_file_cachep, flag);
#define kfs_cache_free_file(file) kfs_cache_free(MEM_FS, kfs_file_cachep, file);
#define kfs_cache_alloc_ioreq(flag) kfs_cache_alloc(MEM_IO, struct kfs_io, kfs_ioreq_cachep, flag);
#define kfs_cache_free_ioreq(ioreq) kfs_cache_free(MEM_IO, kfs_ioreq_cachep, ioreq);
#define kfs_cache_alloc_work(flag) kfs_cache_alloc(MEM_WORK, struct kfs_work, kfs_work_cachep, flag);
#define kfs_cache_free_work(work) kfs_cache_free(MEM_WORK, kfs_work_cachep, work);
#define kfs_cache_alloc_range(flag) kfs_cache_alloc(MEM_IO, struct kfs_file_range, kfs_range_cachep, flag);
#define kfs_cache_free_range(range) kfs_cache_free(MEM_IO, kfs_range_cachep, range);
#endif

#endif //__KFS_MEM_H__
