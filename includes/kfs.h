/* vim: set expandtab ts=4 sw=4:                               */

/*-===========================================================-*/
/*  Author:                                                    */
/*        KevinKW                                              */
/*-===========================================================-*/

/*
 * Define the common structures that are used for KFS mount
 *
 */
#ifndef __KFS_H__
#define __KFS_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <mntent.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <sys/mount.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <features.h>
#include <errno.h>
#include <mntent.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <sys/mount.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <stddef.h>
#include <stdarg.h>
#ifdef KFS_HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <pthread.h>

#include "kfs_opt.h"
#include "kfs_libs.h"


struct kfs_sb {
    u32 magic;
    u32 version;
    u32 dbg_size;
    u32 ibg_size;
    u64 bused;
    u64 iused;
    u64 dbg_num;
    u64 ibg_num;
    u64 mount_times;
    u64 umount_times;
    struct timespec mount_time;
    struct timespec umount_time;
} __attribute__((packed));

#define KFS_BG_INODE    1
#define KFS_BG_DATA     2

struct kfs_bgd {
    u32 type;
    u32 used;
} __attribute__((packed));

struct kfs_bitmap {
    u8 bitmap[KFS_BLOCK_SIZE];
};

#define KFS_IHASH_SLOT 32
struct ihash {
    struct list_head inodes;
    pthread_mutex_t lock;
};

struct kfs_bg {
    u64 bno;
    u64 bid;
    struct kfs *fs;
    struct kfs_bgd bgd;
    struct kfs_bitmap bitmap;
    struct list_head link;
    struct ihash ihash[KFS_IHASH_SLOT];
    pthread_mutex_t lock;
    u32 state;
} __attribute__((packed));

struct kfs_mount_opt {
    u32 flags;
    u32 update_daley;
};

#define kfs_ibg_size(fs)        ((fs)->sb.ibg_size)
#define kfs_dbg_size(fs)        ((fs)->sb.dbg_size)

#define KFS_INIT_BIT     0
#define KFS_OK_BIT       1
#define KFS_DIRTY_BIT    2

struct kfs {
    struct kfs_sb sb;
    struct kfs_mount_opt mntopt;
    struct list_head ibgs;
    struct list_head dbgs;
    pthread_rwlock_t extend_ibg_lock;
    pthread_rwlock_t extend_dbg_lock;
    pthread_rwlock_t sb_lock;
    pthread_mutex_t extend_lock;
    pthread_mutex_t lock;
    u64 filesize;
    u32 state;
    u32 inode_per_bg;
    u32 block_per_bg;
    time_t synctime;
    int fd;
};

struct kfs_node {
    u64 size;
    u32 uid;
    u32 gid;
    u32 mode;
    u32 nlink;
    u32 atime;
    u32 ctime;
    u32 mtime;
    u32 btime;
    u64 pad[16 - 5];
    u64 db[KFS_DB_NUM];
    u64 indb;
};

struct kfs_inode {
    struct kfs_node node;
    struct list_head link;
    pthread_mutex_t lock;
    u64 ino;
    struct kfs_bg *bg;
    u32 state;
};

struct kfs_entry_meta {
    u64 ino;
    u32 type;
    u32 length;
};

struct kfs_dentry {
    struct kfs_dentry *parent;
    struct list_head brothers;
    struct list_head children;
    pthread_mutex_t lock;
    struct kfs_inode *inode;
    struct kfs_entry_meta meta;
    char *name;
    int namelen;
};

/* Using this file to help modify the build options as wanted */
//#include "kfs_build_helper.h"
#endif //__KFS_H__
