/* vim: set expandtab ts=4 sw=4:                               */

/*-===========================================================-*/
/*  Author:                                                    */
/*        KevinKW                                              */
/*-===========================================================-*/

#include <kfs.h>

/*
 * What I plan to do for the block group lib:
 * - get_bg
 * - read_bg
 * - write_bg
 * - alloc_inode
 * - alloc_blocks
 * - new_inode_bg
 * - new_block_bg
 */

u64 bg_offset(struct kfs_bg *bg)
{
    return bg->bno << KFS_BLOCK_SHIFT;
}

u64 bgmap_offset(struct kfs_bg *bg)
{
    return (bg->bno << KFS_BLOCK_SHIFT) + KFS_BGD_SIZE;
}

static int kfs_find_and_set_bitmap(struct kfs_bitmap *bm)
{
    int no = 0;
    u64 *data64 = (u64 *)(bm->bitmap);
    u8 *data8;
    u8 mask = 1;

    while (*data64 == 0xFFFFFFFFFFFFFFFFULL) {
        no += 64;
        data64++;
    }

    data8 = (u8*)data64;
    while (*data8 == 0xFF) {
        no += 8;
        data8++;
    }

    while (*data8 & mask) {
        no++;
        mask <<= 1;
    }

    *data8 |= mask;

    return no;
}

void kfs_init_bg(struct kfs *fs, struct kfs_bg *bg, u64 id, u32 type, u64 offset)
{
    int i;
    memset(bg, 0, sizeof(*bg));
    bg->bid = id;
    bg->bgd.type = type;
    bg->state = 0;

    INIT_LIST_HEAD(&bg->link);
    for (i = 0; i < KFS_IHASH_SLOT; i++) {
        INIT_LIST_HEAD(&bg->ihash[i].inodes);
        pthread_mutex_init(&bg->ihash[i].lock, NULL);
    }
    pthread_mutex_init(&bg->lock, NULL);
    bg->fs = fs;
    bg->bno = (offset >> KFS_BLOCK_SHIFT);
}

int kfs_extend_bg(struct kfs *fs, u32 type)
{
    int ret = 0;
    u64 new_filesize;
    u64 new_id;
    struct kfs_bg *bg;

    bg = kfs_alloc(MEM_FS, sizeof(*bg));
    if (!bg) {
        kerr("Alloc block group object failed\n");
        return -ENOMEM;
    }

    lock_for_extend_fs(fs);
    if (type == KFS_BG_INODE) {
        new_filesize = fs->filesize + kfs_ibg_size(fs) + KFS_BG_META_SIZE;
        new_id = fs->sb.ibg_num;
    } else {
        new_filesize = fs->filesize + kfs_dbg_size(fs) + KFS_BG_META_SIZE;
        new_id = fs->sb.dbg_num;
    }

    ret = ftruncate(fs->fd, new_filesize);
    if (ret < 0) {
        kerr("Extend fs to %llu failed: %s\n",
                new_filesize, strerror(errno));
        goto out;
    }

    kfs_init_bg(fs, bg, new_id, type, fs->filesize);

    ret = pwrite(fs->fd, &bg->bgd, sizeof(bg->bgd), fs->filesize);
    if (ret != sizeof(bg->bgd)) {
        kerr("Write block group discriptor failed %s\n",
                strerror(errno));
        ret = ftruncate(fs->fd, fs->filesize);
        if (ret < 0) {
            kerr("Trucate fs back to %llu failed: %s\n",
                    fs->filesize, strerror(errno));
            mark_fs_err(fs, 0);
            /* Can't fix it */
        }
        ret = -EIO;
        goto out;
    } else {
        ret = 0;
    }

    fs->filesize = new_filesize;

    if (type == KFS_BG_INODE) {
        fs->sb.ibg_num++;
        pthread_rwlock_wrlock(&fs->extend_ibg_lock);
        list_add_tail(&bg->link, &fs->ibgs);
        pthread_rwlock_unlock(&fs->extend_ibg_lock);
    } else {
        fs->sb.dbg_num++;
        pthread_rwlock_wrlock(&fs->extend_dbg_lock);
        list_add_tail(&bg->link, &fs->dbgs);
        pthread_rwlock_unlock(&fs->extend_dbg_lock);
    }
    mark_fs_dirty(fs, 0);

  out:
    unlock_for_extend_fs(fs);
    /* For the success case, the bg will be released by umount */
    if (ret) {
        kfs_free(MEM_FS, bg);
    }
    return ret;
}

void mark_bg_dirty(struct kfs_bg *bg, int locked)
{
    kfs_set_bit(KFS_DIRTY_BIT, &bg->state, locked?NULL:&bg->lock);
    /* Do issue bg update async */
}

int kfs_alloc_inode_bg(struct kfs_bg *ibg, u64 *ino)
{
    int no;

    no = kfs_find_and_set_bitmap(&ibg->bitmap);

    *ino = (ibg->bid * ibg->fs->inode_per_bg) + no;
    ibg->bgd.used++;

    KFS_ASSERT(ibg->bgd.used <= ibg->fs->inode_per_bg);

    mark_bg_dirty(ibg, 1);
    kfs_inc_iused(ibg->fs);

    return 0;
}

int kfs_sync_bg(struct kfs_bg *bg, int locked)
{
    int ret = 0, i;
    struct kfs_inode *inode;

    if (bg->bgd.type == KFS_BG_INODE) {
        for (i = 0; i < KFS_IHASH_SLOT; i++) {
            pthread_mutex_lock(&bg->ihash[i].lock);
            list_for_each_entry(inode, &bg->ihash[i].inodes, link) {
                kfs_lock_inode(inode);
                ret = kfs_sync_inode(inode, 1);
                if (ret) {
                    kfs_unlock_inode(inode);
                    pthread_mutex_unlock(&bg->ihash[i].lock);
                    goto out;
                }
                kfs_unlock_inode(inode);
            }
            pthread_mutex_unlock(&bg->ihash[i].lock);
        }
    }

    if (kfs_test_bit(KFS_DIRTY_BIT, &bg->state, locked?NULL:&bg->lock)) {
        ret = pwrite(bg->fs->fd, &bg->bgd, sizeof(bg->bgd), bg_offset(bg));
        if (ret != sizeof(bg->bgd)) {
            kerr("Write block group discriptor failed %s\n",
                    strerror(errno));
            ret = -EIO;
            goto out;
        }
        ret = pwrite(bg->fs->fd, &bg->bitmap, sizeof(bg->bitmap), bgmap_offset(bg));
        if (ret != sizeof(bg->bitmap)) {
            kerr("Write block group bitmap failed %s\n",
                    strerror(errno));
            ret = -EIO;
            goto out;
        } else {
            ret = 0;
        }
    }

out:
    return ret;
}

int kfs_sync_bgs(struct kfs *fs, u32 type)
{
    int ret = 0;
    struct kfs_bg *bg;
    struct list_head *bgs;

    if (type == KFS_BG_INODE) {
        bgs = &fs->ibgs;
    } else {
        bgs = &fs->dbgs;
    }
    lock_bgs(fs, type);
    list_for_each_entry(bg, bgs, link) {
        lock_bg(bg);
        ret = kfs_sync_bg(bg, 1);
        unlock_bg(bg);
        if (ret) {
            break;
        }
    }
    unlock_bgs(fs, type);

    return ret;
}

/* Return the locked ibg */
struct kfs_bg *kfs_get_ibg(struct kfs *fs, u64 ino)
{
    struct kfs_bg *ibg;
    int found = 0;
    int bid = ino / fs->inode_per_bg;

    lock_bgs(fs, KFS_BG_INODE);
    list_for_each_entry(ibg, &fs->ibgs, link) {
        if (ibg->bid == bid) {
            found = 1;
            break;
        }
    }
    unlock_bgs(fs, KFS_BG_INODE);

    if (found) {
        kdebug(LOG_VFS, "get ibg %p id %llu for ino %llu\n",
                ibg, ibg->bid, ino);
        return ibg;
    } else {
        kerr("Can't get the ibg for inode %llu\n", ino);
    }

    return NULL;
}
