/* vim: set expandtab ts=4 sw=4:                               */

/*-===========================================================-*/
/*  Author:                                                    */
/*        KevinKW                                              */
/*-===========================================================-*/

#include <kfs.h>

/*
 * What I plan to do for the block group lib:
 * - read_sb
 * - write_sb
 * - alloc_inode
 * - alloc_blocks
 * - new_sb
 */

void kfs_init(struct kfs *fs)
{
    memset(fs, 0, sizeof(*fs));
    INIT_LIST_HEAD(&fs->ibgs);
    INIT_LIST_HEAD(&fs->dbgs);
    pthread_rwlock_init(&fs->extend_ibg_lock, NULL);
    pthread_rwlock_init(&fs->extend_dbg_lock, NULL);
    pthread_rwlock_init(&fs->sb_lock, NULL);
    pthread_mutex_init(&fs->extend_lock, NULL);
    pthread_mutex_init(&fs->lock, NULL);
}

void mark_fs_ok(struct kfs *fs, int locked)
{
    kfs_set_bit(KFS_OK_BIT, &fs->state, locked?NULL:&fs->lock);
}

void mark_fs_err(struct kfs *fs, int locked)
{
    kfs_clear_bit(KFS_OK_BIT, &fs->state, locked?NULL:&fs->lock);
}

void mark_fs_dirty(struct kfs *fs, int locked)
{
    kfs_set_bit(KFS_DIRTY_BIT, &fs->state, locked?NULL:&fs->lock);
    /* Do issue sb update async */
}

int kfs_sync_sb(struct kfs *fs)
{
    int ret = 0;

    if (kfs_test_bit(KFS_DIRTY_BIT, &fs->state, &fs->lock)) {
        ret = pwrite(fs->fd, &fs->sb, sizeof(fs->sb), 0);
        if (ret != sizeof(fs->sb)) {
            kerr("Sync superblock failed %s\n",
                    strerror(errno));
            ret = -EIO;
        } else {
            fs->synctime = time(NULL);
            kdebug(LOG_IO, "Sync sb time %ld\n", fs->synctime);
            ret = 0;
        }
    }

    return ret;
}

int kfs_alloc_ino(struct kfs *fs, struct kfs_inode *inode)
{
    int ret;
    int found = 0;
    struct kfs_bg *ibg;

  retry:
    lock_bgs(fs, KFS_BG_INODE);
    list_for_each_entry(ibg, &fs->ibgs, link) {
        lock_bg(ibg);
        if (ibg->bgd.used < fs->inode_per_bg) {
            found = 1;
            break;
        }
        unlock_bg(ibg);
    }

    if (!found) {
        unlock_bgs(fs, KFS_BG_INODE);
        kinfo("No available inode group, trying to extend filesystem...\n");
        ret = kfs_extend_bg(fs, KFS_BG_INODE);
        if (ret) {
            return ret;
        }
        goto retry;
    }

    kdebug(LOG_OBJECT, "Found one bg %p used %u\n",
            ibg, ibg->bgd.used);

    ret = kfs_alloc_inode_bg(ibg, &inode->ino);
    if (!ret) {
        kfs_ihash_insert(ibg, inode, 0);
        inode->bg = ibg;
    }
    unlock_bg(ibg);
    unlock_bgs(fs, KFS_BG_INODE);

    return ret;
}

int kfs_alloc_inode(struct kfs *fs, struct kfs_inode **inodep)
{
    struct kfs_inode *inode;
    int ret;

    inode = kfs_alloc(MEM_FS, sizeof(*inode));
    if (!inode) {
        kerr("Allocate inode object failed\n");
        return -ENOMEM;
    }

    kfs_init_inode(inode);

    ret = kfs_alloc_ino(fs, inode);
    if (ret) {
        kerr("Allocate inode number failed\n");
        kfs_free(MEM_FS, inode);
        return ret;
    }

    kfs_set_bit(KFS_INIT_BIT, &inode->state, &inode->lock);
    *inodep = inode;

    return 0;
}

int kfs_sync_fs(struct kfs *fs)
{
    int ret;

    lock_for_extend_fs(fs);
    ret = kfs_sync_bgs(fs, KFS_BG_INODE);
    if (ret) {
        goto out;
    }

    ret = kfs_sync_bgs(fs, KFS_BG_DATA);
    if (ret) {
        goto out;
    }

    ret = kfs_sync_sb(fs);
  out:
    unlock_for_extend_fs(fs);
    return ret;
}

int kfs_read_sb(struct kfs *fs)
{
    int ret;
    struct stat st;

    ret = pread(fs->fd, &fs->sb, sizeof(fs->sb), 0);
    if (ret != sizeof(fs->sb)) {
        kerr("Read super block failed\n");
        return -EIO;
    }

    if (fs->sb.magic != KFS_SB_MAGIC
            || fs->sb.version != KFS_SB_VERSION) {
        kerr("Super block check failed\n");
        return -EINVAL;
    }

    ret = fstat(fs->fd, &st);
    if (ret < 0) {
        kerr("Get filesize failed: %s\n", strerror(errno));
        return -EIO;
    }

    fs->filesize = st.st_size;
    if (fs->filesize !=
            (KFS_SB_SIZE
             + (KFS_BGD_SIZE * (fs->sb.ibg_num+fs->sb.dbg_num))
             + (KFS_BITMAP_SIZE * (fs->sb.ibg_num+fs->sb.dbg_num))
             + (fs->sb.ibg_size * fs->sb.ibg_num)
             + (fs->sb.dbg_size * fs->sb.dbg_num))) {
        kerr("Check filesize failed: %s\n", strerror(errno));
        return -EINVAL;
    }

    fs->inode_per_bg = kfs_ibg_size(fs) / KFS_INODE_SIZE;
    fs->block_per_bg = kfs_dbg_size(fs) / KFS_BLOCK_SIZE;

    kdebug(LOG_VFS, "Read sb iused %llu bused %llu\n",
            fs->sb.iused, fs->sb.bused);
    return 0;
}

/* This will be done during mount time, so no lock is needed */
int kfs_build_bgs(struct kfs *fs)
{
    struct kfs_bg *bg;
    int ret;
    u64 offset = KFS_SB_SIZE;
    u64 ibgid = 0;
    u64 dbgid = 0;


    while (offset < fs->filesize) {
        if ((fs->filesize - offset) < KFS_BGD_SIZE) {
            kerr("Invalid filesize left offset %llu filesize %llu\n",
                    offset, fs->filesize);
            return -EINVAL;
        }

        bg = kfs_alloc(MEM_FS, sizeof(*bg));
        if (!bg) {
            kerr("Alloc bg failed\n");
            return -ENOMEM;
        }

        /* I don't know the bg type yet, will set it later */
        kfs_init_bg(fs, bg, 0, 0, offset);

        ret = pread(fs->fd, &bg->bgd, sizeof(bg->bgd), offset);
        if (ret != sizeof(bg->bgd)) {
            kerr("Read block group discriptor failed %s\n",
                    strerror(errno));
            return -EIO;
        }

        if (bg->bgd.type == KFS_BG_INODE) {
            bg->bid = ibgid;
            ibgid++;
        } else {
            bg->bid = dbgid;
            dbgid++;
        }

        offset += KFS_BGD_SIZE;
        if ((fs->filesize - offset) < KFS_BITMAP_SIZE) {
            kerr("Invalid filesize left offset %llu filesize %llu\n",
                    offset, fs->filesize);
            return -EINVAL;
        }

        ret = pread(fs->fd, &bg->bitmap, sizeof(bg->bitmap), offset);
        if (ret != sizeof(bg->bitmap)) {
            kerr("Read block group bitmap failed %s\n",
                    strerror(errno));
            return -EIO;
        }

        offset += KFS_BITMAP_SIZE;

        offset += (bg->bgd.type == KFS_BG_INODE)?fs->sb.ibg_size:fs->sb.dbg_size;

        if (offset > fs->filesize) {
            kerr("Invalid filesize left offset %llu filesize %llu\n",
                    offset, fs->filesize);
            return -EINVAL;
        }

        if (bg->bgd.type == KFS_BG_INODE) {
            list_add_tail(&bg->link, &fs->ibgs);
        } else {
            list_add_tail(&bg->link, &fs->dbgs);
        }
        kdebug(LOG_VFS, "Init bg type %u id %llu\n",
                bg->bgd.type, bg->bid);
    }

    return 0;
}

void kfs_inc_iused(struct kfs *fs)
{
    pthread_rwlock_wrlock(&fs->sb_lock);
    fs->sb.iused++;
    pthread_rwlock_unlock(&fs->sb_lock);
    mark_fs_dirty(fs, 0);
}

void kfs_dec_iused(struct kfs *fs)
{
    pthread_rwlock_wrlock(&fs->sb_lock);
    fs->sb.iused--;
    pthread_rwlock_unlock(&fs->sb_lock);
    mark_fs_dirty(fs, 0);
}
