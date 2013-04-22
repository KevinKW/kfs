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

u64 inode_offset(struct kfs_inode *inode)
{
    return bg_offset(inode->bg) + ((inode->ino%inode->bg->fs->inode_per_bg) << KFS_INODE_SHIFT) + KFS_BG_META_SIZE;
}

void kfs_fill_inode(struct kfs_inode *inode)
{
}

void kfs_lock_inode(struct kfs_inode *inode)
{
    pthread_mutex_lock(&inode->lock);
}

void kfs_unlock_inode(struct kfs_inode *inode)
{
    pthread_mutex_unlock(&inode->lock);
}

void mark_inode_dirty(struct kfs_inode *inode, int locked)
{
    kfs_set_bit(KFS_DIRTY_BIT, &inode->state, locked?NULL:&inode->lock);
}

int kfs_sync_inode(struct kfs_inode *inode, int locked)
{
    int ret = 0;

    if (kfs_test_bit(KFS_DIRTY_BIT, &inode->state, locked?NULL:&inode->lock)) {
        ret = pwrite(inode->bg->fs->fd, &inode->node, sizeof(inode->node),
                inode_offset(inode));
        if (ret != sizeof(inode->node)) {
            kerr("Write inode failed %s\n",
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

int kfs_read_inode(struct kfs_inode *inode)
{
    int ret;

    ret = pread(inode->bg->fs->fd, &inode->node, sizeof(inode->node), inode_offset(inode));
    if (ret != sizeof(inode->node)) {
        kerr("Read inode failed %s\n",
                strerror(errno));
        ret = -EIO;
        goto out;
    } else {
        ret = 0;
    }
  out:
    return ret;
}

void kfs_init_inode(struct kfs_inode *inode)
{
    memset(inode, 0, sizeof(*inode));
    INIT_LIST_HEAD(&inode->link);
    pthread_mutex_init(&inode->lock, NULL);
}

void kfs_ihash_insert(struct kfs_bg *ibg, struct kfs_inode *inode, int locked)
{
    int slot = inode->ino/KFS_IHASH_SLOT;

    if (!locked) {
        pthread_mutex_lock(&ibg->ihash[slot].lock);
    }
    list_add_tail(&inode->link, &ibg->ihash[slot].inodes);
    if (!locked) {
        pthread_mutex_unlock(&ibg->ihash[slot].lock);
    }
}

void kfs_ihash_remove(struct kfs_bg *ibg, struct kfs_inode *inode, int locked)
{
    int slot = inode->ino/KFS_IHASH_SLOT;

    if (!locked) {
        pthread_mutex_lock(&ibg->ihash[slot].lock);
    }
    list_del_init(&inode->link);
    if (!locked) {
        pthread_mutex_unlock(&ibg->ihash[slot].lock);
    }
}

struct kfs_inode *kfs_ihash_get(struct kfs_bg *ibg, u64 ino)
{
    int slot = ino/KFS_IHASH_SLOT;
    struct kfs_inode *inode;

    pthread_mutex_lock(&ibg->ihash[slot].lock);
    list_for_each_entry(inode, &ibg->ihash[slot].inodes, link) {
        if (inode->ino == ino) {
            pthread_mutex_unlock(&ibg->ihash[slot].lock);
            kfs_lock_inode(inode);
            return inode;
        }
    }

    inode = kfs_alloc(MEM_FS, sizeof(*inode));
    if (!inode) {
        kerr("Alloc inode failed\n");
        pthread_mutex_unlock(&ibg->ihash[slot].lock);
        return NULL;
    }

    kfs_init_inode(inode);
    inode->ino = ino;
    inode->bg = ibg;
    kfs_ihash_insert(ibg, inode, 1);

    pthread_mutex_unlock(&ibg->ihash[slot].lock);

    return inode;
}

struct kfs_inode *kfs_get_inode(struct kfs *fs, u64 ino)
{
    struct kfs_inode *inode;
    struct kfs_bg *ibg;
    int ret;

    ibg = kfs_get_ibg(fs, ino);
    if (!ibg) {
        return NULL;
    }

    inode = kfs_ihash_get(ibg, ino);
    if (!inode) {
        goto out;
    }

    if (!kfs_test_bit(KFS_INIT_BIT, &inode->state, NULL)) {
        /* Do read from file */
        if (kfs_read_inode(inode)) {
            /* FIXME: Inode need ref here */
            kfs_ihash_remove(ibg, inode, 0);
        }
    }

    kfs_unlock_inode(inode);

  out:
    unlock_bgs(fs, KFS_BG_INODE);
    return inode;
}
