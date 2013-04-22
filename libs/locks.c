/* vim: set expandtab ts=4 sw=4:                               */

/*-===========================================================-*/
/*  Author:                                                    */
/*        KevinKW                                              */
/*-===========================================================-*/


/*
 * What I plan to do for the lock lib:
 * - lock_for_extent_fs
 * - lock_bg
 */
#include <kfs.h>

void lock_for_extend_fs(struct kfs *fs)
{
    pthread_mutex_lock(&fs->extend_lock);
}

void unlock_for_extend_fs(struct kfs *fs)
{
    pthread_mutex_unlock(&fs->extend_lock);
}

void lock_bgs(struct kfs *fs, u32 type)
{
    if (type == KFS_BG_INODE) {
        pthread_rwlock_rdlock(&fs->extend_ibg_lock);
    } else {
        pthread_rwlock_rdlock(&fs->extend_dbg_lock);
    }
}

void unlock_bgs(struct kfs *fs, u32 type)
{
    if (type == KFS_BG_INODE) {
        pthread_rwlock_unlock(&fs->extend_ibg_lock);
    } else {
        pthread_rwlock_unlock(&fs->extend_dbg_lock);
    }
}

void lock_bg(struct kfs_bg *bg)
{
    pthread_mutex_lock(&bg->lock);
}

void unlock_bg(struct kfs_bg *bg)
{
    pthread_mutex_unlock(&bg->lock);
}
