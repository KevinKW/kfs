/* vim: set expandtab ts=4 sw=4:                               */

/*-===========================================================-*/
/*  Author:                                                    */
/*        KevinKW                                              */
/*-===========================================================-*/

#include <kfs.h>

void kfs_init_dentry(struct kfs_dentry *dentry, char *name, int namelen)
{
    dentry->parent = NULL;
    INIT_LIST_HEAD(&dentry->brothers);
    INIT_LIST_HEAD(&dentry->children);
    memset(&dentry->meta, 0, sizeof(dentry->meta));
    dentry->inode = NULL;
    pthread_mutex_init(&dentry->lock, NULL);
    dentry->name = ((char *)dentry + sizeof(*dentry));
    dentry->namelen = namelen;
    sprintf(dentry->name, "%s", name);
}

struct kfs_dentry *kfs_alloc_dentry(char *name)
{
    struct kfs_dentry *dentry;
    int namelen = strlen(name);

    dentry = malloc(sizeof(*dentry) + namelen);
    if (!dentry) {
        return NULL;
    }

    kfs_init_dentry(dentry, name, namelen);

    kdebug(LOG_VFS, "alloc dentry %p name %s\n", dentry, dentry->name);

    return dentry;
}

void kfs_free_dentry(struct kfs_dentry *dentry)
{
    kdebug(LOG_VFS, "free dentry %p name %s\n", dentry, dentry->name);

    if (dentry->name != ((char *)dentry + sizeof(*dentry))) {
        free(dentry->name);
    }

    free(dentry);
}

void lock_dentry(struct kfs_dentry *dentry)
{
    pthread_mutex_lock(&dentry->lock);
}

void unlock_dentry(struct kfs_dentry *dentry)
{
    pthread_mutex_unlock(&dentry->lock);
}

/* Parent must be locked */
/* Return locked dentry if found */
struct kfs_dentry *kfs_find_dentry(struct kfs_dentry *parent, char *name)
{
    struct kfs_dentry *dentry;
    int found, namelen = strlen(name);

    list_for_each_entry(dentry, &parent->children, brothers) {
        lock_dentry(dentry);
        if (dentry->namelen != namelen) {
            unlock_dentry(dentry);
            continue;
        }

        if (strcmp(dentry->name, name) == 0) {
            found = 1;
            break;
        }
        unlock_dentry(dentry);
    }

    return found?dentry:NULL;
}
