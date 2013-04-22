/* vim: set expandtab ts=4 sw=4:                               */

/*-===========================================================-*/
/*  Author:                                                    */
/*        KevinKW                                              */
/*-===========================================================-*/

/*
 * KFS in FUSE
 *
 */

#define FUSE_USE_VERSION 30

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if 0
#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif
#endif


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
#include <sys/time.h>
#include <stddef.h>
#include <stdarg.h>
#ifdef KFS_HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <pthread.h>
#include <fuse.h>
#include <kfs.h>

struct kfs fs;
struct kfs_dentry root;

struct kfs_params {
    char *filename;
    int logLevel;
} kfs_param = {
    .filename = NULL,
    .logLevel = DEFAULT_LOG_LEVEL
};

#define KFS_OPT(t, p) { t, offsetof(struct kfs_params, p), 1 }

static const struct fuse_opt kfs_opts[] = {
    KFS_OPT("-f %s", filename),
    KFS_OPT("-l %d", logLevel),
    FUSE_OPT_END
};

static int kfs_process_arg(void *data, const char *arg, int key,
        struct fuse_args *outargs)
{
    (void)outargs;
    (void)arg;

    kdebug(LOG_VFS, "key %d arg: %s\n", key, arg);

    switch (key) {
        case 0:
            return 0;
        default:
            return 1;
    }
}

#define KFS_LOOKUP_PARENT       0x0001

static int kfs_dentry_lookup(struct kfs *fs, struct kfs_dentry *dir,
        char *name, int namelen, struct kfs_dentry **dp)
{
    int ret;
    struct kfs_dentry *dentry;
}

static int kfs_lookup(struct kfs *fs, const char *path,
        struct kfs_dentry **dp, int flags)
{
    int ret = -ENOENT;
    int namelen = strlen(path);

    if (namelen == 1 && path[0] == '/') {
        kdebug(LOG_VFS, "Return root\n");
        *dp = &root;
        return 0;
    }

    if (flags & KFS_LOOKUP_PARENT) {
        return &root;
    }
    kdebug(LOG_VFS, "Path %s not found\n", path);
    return ret;
}

static int kfs_getattr(const char *path, struct stat *stbuf)
{
    int ret;
    struct kfs_dentry *dentry;
    struct kfs_inode *inode;

    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);

    ret = kfs_lookup(&fs, path, &dentry, 0);
    if (ret) {
        return ret;
    }

    inode = dentry->inode;

    kdebug3(LOG_VFS, "fill attr\n");
    stbuf->st_dev = 200;
    stbuf->st_ino = inode->ino;
    stbuf->st_mode = inode->node.mode;
    stbuf->st_nlink = inode->node.nlink;
    stbuf->st_uid = inode->node.uid;
    stbuf->st_gid = inode->node.gid;
    stbuf->st_rdev = 0;
    stbuf->st_size = inode->node.size;
    stbuf->st_blksize = 512;
    stbuf->st_blocks = (inode->node.size+511) >> 9;
    stbuf->st_atime = inode->node.mtime;
    stbuf->st_mtime = inode->node.mtime;
    stbuf->st_ctime = inode->node.ctime;

    kdebug(LOG_VFS, "attr inode %lu mode %o\n",
            stbuf->st_ino, stbuf->st_mode);

    return 0;
}

static int kfs_access(const char *path, int mask)
{
    int ret;

    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);

    return 0;
    ret = access(path, mask);
    if (ret == -1)
        return -errno;

    return 0;
}

static int kfs_readlink(const char *path, char *buf, size_t size)
{
    int ret;
    struct kfs_dentry *dentry;
    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);

    ret = kfs_lookup(&fs, path, &dentry, 0);
    if (ret < 0) {
        return ret;
    }

    buf[ret] = '\0';
    return 0;
}

struct kfs_dir_info {
    u64 ino;
    u32 type;
    off_t offset;
    struct rest_dir_entry *de;
    char *demem;
};

static int kfs_disk_Readdir(struct kfs *fs, const char *path, struct kfs_dir_info *di)
{
    int ret = -EIO;
    return ret;
}

static int kfs_opendir(const char *path, struct fuse_file_info *fi)
{
    int ret;
    struct kfs_dentry *dentry;
    struct kfs_inode *inode;

    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);

    ret = kfs_lookup(&fs, path, &dentry, 0);
    if (ret < 0) {
        return ret;
    }

    inode = dentry->inode;

#if 0
    //di = kfs_alloc(MEM_FS, sizeof(struct kfs_dir_info), 0);
    if (!di) {
        kerr("Allocate dir info failed\n");
        return -ENOMEM;
    }
    di->ino = inode->ino;
    di->type = inode->node.mode;
    di->offset = 0;
    di->de = NULL;
    di->demem = NULL;

    fi->fh = (unsigned long)di;
#endif
    return 0;
}

static int kfs_releasedir(const char *path, struct fuse_file_info *fi)
{
    struct kfs_dir_info *di = (struct kfs_dir_info*)fi->fh;

    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);

    if (di->demem) {
        kfs_free(MEM_FS, di->demem);
    }
    fi->fh = 0;
    return 0;
}

static int kfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
               off_t offset, struct fuse_file_info *fi)
{
    int ret;
    struct kfs_dir_info *di = (struct kfs_dir_info*)fi->fh;
    char *p;

    (void) offset;
    (void) fi;

    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);

    if (!di->demem) {
        //ret = kfs_disk_Readdir(&fs, path, di);
        if (ret < 0)
            return ret;
    } else {
        /* check offset */
    }

#if 0
    de = di->de;

    while (de->ino != 0) {
        struct stat st;
        char name[256];
        memset(&st, 0, sizeof(st));
        st.st_ino = de->ino;
        st.st_mode = de->type;
        snprintf(name, de->namelen+1, "%s", de->name);
        kdebug(LOG_PROTOCOL, "entry name %s, ino %llu, type %o\n",
                name, de->ino, de->type);
        if (filler(buf, name, &st, 0))
            break;
        p = (char *)de;
        p += de->length;
        de = (struct rest_dir_entry*)p;
    }
#endif

    return 0;
}

static int kfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    int ret;

    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);
    /* On Linux this could just be 'mknod(path, mode, rdev)' but this
       is more portable */

    return -EPERM;

    if (S_ISREG(mode)) {
        ret = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (ret >= 0)
            ret = close(ret);
    } else if (S_ISFIFO(mode))
        ret = mkfifo(path, mode);
    else
        ret = mknod(path, mode, rdev);
    if (ret == -1)
        return -errno;

    return 0;
}


static int kfs_mkdir(const char *path, mode_t mode)
{
    int ret;
    struct kfs_dentry *dentry;
    char target_path[1024];

    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);

    if (strlen(path) > (1024 - 2)) {
        return -ENAMETOOLONG;
    }

    sprintf(target_path, "%s/", path);
#if 0
    attr.uid = fuse_get_context()->uid;
    attr.gid = fuse_get_context()->gid;
    attr.mode = mode;

    ret = kfs_disk_Create(&fs, target_path, &attr);
#endif
    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int kfs_unlink(const char *path)
{
    int ret;
    struct kfs_dentry *dentry;
    struct kfs_inode *inode;
    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);

    ret = kfs_lookup(&fs, path, &dentry, 0);
    if (ret < 0) {
        return ret;
    }

    //ret = kfs_disk_Remove(&fs, path, &attr);
    if (ret < 0) {
        return ret;
    }

    return 0;
}


static int kfs_rmdir(const char *path)
{
    int ret;
    struct kfs_dentry *dentry;
    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);

    ret = kfs_lookup(&fs, path, &dentry, 0);
    if (ret < 0) {
        return ret;
    }

    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int kfs_rename(const char *from, const char *to)
{
    int ret;

    kdebug(LOG_VFS, "%s: from %s to %s\n", __FUNCTION__, from, to);


    return 0;
}


static int kfs_disk_Symlink(struct kfs *fs, const char *from,
        const char *to)
{
    int ret = -EIO;
    return ret;
}

static int kfs_symlink(const char *from, const char *to)
{
    int ret;
    kdebug(LOG_VFS, "%s: from %s to %s\n", __FUNCTION__, from, to);

    ret = kfs_disk_Symlink(&fs, from, to);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int kfs_link(const char *from, const char *to)
{
    int ret;
    kdebug(LOG_VFS, "%s: from %s to %s\n", __FUNCTION__, from, to);

    return -EPROTONOSUPPORT;

    ret = link(from, to);
    if (ret == -1)
        return -errno;

    return 0;
}


static int kfs_chmod(const char *path, mode_t mode)
{
    int ret; 
    struct kfs_dentry *dentry;
    struct kfs_inode *inode;

    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);

    ret = kfs_lookup(&fs, path, &dentry, 0);
    if (ret < 0) { 
        return ret; 
    }    

    if (ret < 0) { 
        return ret; 
    }    

    return 0;

}

static int kfs_chown(const char *path, uid_t uid, gid_t gid)
{
    int ret;
    struct kfs_dentry *dentry;

    kdebug(LOG_VFS, "%s: path %s uid %d gid %d\n",
            __FUNCTION__, path, uid, gid);

    ret = kfs_lookup(&fs, path, &dentry, 0);
    if (ret < 0) {
        return ret;
    }

    if (ret < 0) {
        return ret;
    }

    return 0;
}


static int kfs_truncate(const char *path, off_t size)
{
    int ret;
    struct kfs_dentry *dentry;

    kdebug(LOG_VFS, "%s: path %s, size %llu\n",
            __FUNCTION__, path, (u64)size);

    ret = kfs_lookup(&fs, path, &dentry, 0);
    if (ret < 0) {
        return ret;
    }

    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int kfs_utimens(const char *path, const struct timespec ts[2])
{
    int ret;
    struct kfs_dentry *dentry;

    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);

    ret = kfs_lookup(&fs, path, &dentry, 0);
    if (ret < 0) {
        return ret;
    }

    if (ret < 0) {
        return ret;
    }

    return 0;
}

struct kfs_file_info {
    struct kfs_dentry *dentry;
};

static int kfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int ret;
    struct kfs_dentry *dentry;
    struct kfs_inode *inode;

    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);

    ret = kfs_lookup(&fs, path, &dentry, KFS_LOOKUP_PARENT);
    if (ret) {
        return ret;
    }

    fi->fh = 0;

    return 0;
}

static int kfs_open(const char *path, struct fuse_file_info *fi)
{
    int ret;

    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);

    fi->fh = 0;


    return 0;
}

static int kfs_disk_Read(struct kfs *fs, const char *path,
        struct kfs_file_info *file, char *buf, size_t size, u64 offset)
{
    int ret = -EIO;
    return ret;
}

static int kfs_disk_Write(struct kfs *fs, const char *path,
        struct kfs_file_info *file, const char *buf, size_t size, u64 offset)
{
    int ret = -EIO;
    return ret;
}

static int kfs_read(const char *path, char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi)
{
    int ret;
    struct kfs_file_info *file = (struct kfs_file_info *)fi->fh;

    kdebug(LOG_VFS, "%s: path %s offset %lu - %lu size %zd\n",
            __FUNCTION__, path, offset, offset+size, size);

    KFS_ASSERT(!(offset & 0x1FF));

    if (!file) {
        kerr("File %s not opened\n", path);
        return -EACCES;
    }

    ret = kfs_disk_Read(&fs, path, file, buf, size, offset);
    if (ret < 0) {
        return ret;
    }
    return size;
}

static int kfs_write(const char *path, const char *buf, size_t size,
             off_t offset, struct fuse_file_info *fi)
{
    int ret;
    struct kfs_file_info *file = (struct kfs_file_info *)fi->fh;
    off_t noffset = offset;
    size_t nsize = size;
    char *nbuf = (char *)buf;
    off_t ds = (offset & 0x1FF);
    off_t de = ((offset+size) & 0x1FF);

    if (de) {
        de = 512 - de;
    }

    kdebug(LOG_VFS, "%s: path %s offset %lu - %lu size %zd\n",
            __FUNCTION__, path, offset, offset+size, size);
    if (!file) {
        kerr("File %s not opened\n", path);
        return -EACCES;
    }

    /* Only do get filesize if de is not 0 */
    if (de) {
        //ret = kfs_disk_Getattr(&fs, path, &file->attr);
        if (ret < 0) {
            return ret;
        }
        /* If this write is going to extend filesize, then no
         * need to be align.
         */
#if 0
        if ((offset + size) >= file->attr.size) {
            de = 0;
        }
#endif
    }

    kdebug2(LOG_VFS, "ds %lu de %lu\n", ds, de);
    //kdebug2(LOG_VFS, "old size %llu newsize %lu\n", file->attr.size, (offset+size));

    if (ds || de) {
        noffset &= (~0x1FF);
        nsize += ds;
        nsize = (nsize + 511) & (~0x1FF);
        nbuf = kfs_alloc(MEM_IO, nsize);
        if (!nbuf) {
            kerr("Allocate IO buffer failed\n");
            return -ENOMEM;
        }
        kdebug(LOG_VFS, "do read on %s ds %lu noffset %lu - %lu nsize %zd\n",
                path, ds, noffset, noffset+nsize, nsize);
        ret = kfs_disk_Read(&fs, path, file, nbuf, nsize, noffset);
        if (ret < 0) {
            kfs_free(MEM_IO, nbuf);
            return ret;
        }
        memcpy(nbuf + ds, buf, size);
        nsize = size+ds+de;
    }

    kdebug(LOG_VFS, "do write on %s ds %lu noffset %lu - %lu nsize %zd\n",
            path, ds, noffset, noffset+nsize, nsize);
    ret = kfs_disk_Write(&fs, path, file, nbuf, nsize, noffset);
    if (ret < 0) {
        if (ds) {
            kfs_free(MEM_IO, nbuf);
        }
        return ret;
    }

    if (ds) {
        kfs_free(MEM_IO, nbuf);
    }
    return size;
}

static int kfs_disk_StatFS (struct kfs *fs)
{
    int ret = -EIO;
    return ret;
}

static int kfs_statfs(const char *path, struct statvfs *stbuf)
{
    int ret;
    unsigned char blockbits;
    unsigned long blockres;

    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);

    stbuf->f_frsize = KFS_BLOCK_SIZE;
    stbuf->f_bsize = KFS_BLOCK_SIZE;
    blockbits = KFS_BLOCK_SHIFT;
    blockres = (1 << blockbits) - 1;
    stbuf->f_blocks = (fs.filesize + blockres) >> blockbits;
    stbuf->f_bfree = stbuf->f_blocks - fs.sb.bused;
    stbuf->f_bavail = ((fs.sb.dbg_num * fs.sb.dbg_size) >> blockbits) - fs.sb.bused;
    stbuf->f_files = fs.sb.ibg_num * fs.inode_per_bg;
    stbuf->f_ffree = (fs.sb.ibg_num * fs.inode_per_bg) - fs.sb.iused;
    kdebug(LOG_VFS, "iuse %llu\n", fs.sb.iused);

    return 0;
}

static int kfs_release(const char *path, struct fuse_file_info *fi)
{
    struct kfs_file_info *file = (struct kfs_file_info *)fi->fh;

    if (file) {
        kfs_free(MEM_FS, file);
        fi->fh = 0;
    }
    return 0;
}

static int kfs_fsync(const char *path, int isdatasync,
             struct fuse_file_info *fi)
{
    /* Just a stub.     This method is optional and can safely be left
       unimplemented */

    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);
    (void) path;
    (void) isdatasync;
    (void) fi;
    return 0;
}

#ifdef KFS_SUPPORT_PREALLOC
static int kfs_fallocate(const char *path, int mode,
            off_t offset, off_t length, struct fuse_file_info *fi)
{
    int fd;
    int ret;
    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);

    return -EPROTONOSUPPORT;
    (void) fi;

    if (mode)
        return -EOPNOTSUPP;

    fd = open(path, O_WRONLY);
    if (fd == -1)
        return -errno;

    ret = -posix_fallocate(fd, offset, length);

    close(fd);
    return ret;
}
#endif

#ifdef KFS_HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int kfs_setxattr(const char *path, const char *name, const char *value,
            size_t size, int flags)
{
    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);
    return -EPROTONOSUPPORT;
    int ret = lsetxattr(path, name, value, size, flags);
    if (ret == -1)
        return -errno;
    return 0;
}

static int kfs_getxattr(const char *path, const char *name, char *value,
            size_t size)
{
    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);
    return -EPROTONOSUPPORT;
    int ret = lgetxattr(path, name, value, size);
    if (ret == -1)
        return -errno;
    return ret;
}

static int kfs_listxattr(const char *path, char *list, size_t size)
{
    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);
    return -EPROTONOSUPPORT;
    int ret = llistxattr(path, list, size);
    if (ret == -1)
        return -errno;
    return ret;
}

static int kfs_removexattr(const char *path, const char *name)
{
    kdebug(LOG_VFS, "%s: path %s\n", __FUNCTION__, path);
    return -EPROTONOSUPPORT;
    int ret = lremovexattr(path, name);
    if (ret == -1)
        return -errno;
    return 0;
}
#endif /* KFS_HAVE_SETXATTR */

static struct fuse_operations kfs_operations = {
    .getattr    = kfs_getattr,
    .access        = kfs_access,
    .readlink    = kfs_readlink,
    .opendir    = kfs_opendir,
    .readdir    = kfs_readdir,
    .releasedir = kfs_releasedir,
    .mknod        = kfs_mknod,
    .mkdir        = kfs_mkdir,
    .symlink    = kfs_symlink,
    .unlink        = kfs_unlink,
    .rmdir        = kfs_rmdir,
    .rename        = kfs_rename,
    .link        = kfs_link,
    .chmod        = kfs_chmod,
    .chown        = kfs_chown,
    .truncate    = kfs_truncate,
    .utimens    = kfs_utimens,
    .create      = kfs_create,
    .open        = kfs_open,
    .read        = kfs_read,
    .write        = kfs_write,
    .statfs        = kfs_statfs,
    .release    = kfs_release,
    .fsync        = kfs_fsync,
#ifdef KFS_SUPPORT_PREALLOC
    .fallocate    = kfs_fallocate,
#endif
#ifdef KFS_HAVE_SETXATTR
    .setxattr    = kfs_setxattr,
    .getxattr    = kfs_getxattr,
    .listxattr    = kfs_listxattr,
    .removexattr    = kfs_removexattr,
#endif
};

static int kfs_mount()
{
    int ret;

    kfs_init(&fs);

    fs.fd = open(kfs_param.filename, O_RDWR|O_NOFOLLOW);
    if (fs.fd < 0) {
        ret = -errno;
        kerr("Open file %s failed: %s\n", kfs_param.filename, strerror(ret));
        return ret;
    }

    ret = kfs_read_sb(&fs);
    if (ret < 0) {
        goto err;
    }

    ret = kfs_build_bgs(&fs);
    if (ret < 0) {
        goto err;
    }

    kfs_init_dentry(&root, "/", 1);
    root.parent = &root;
    root.meta.ino = 0;
    root.meta.type = S_IFDIR;
    root.meta.length = 2 + sizeof(root.meta);

    root.inode = kfs_get_inode(&fs, 0);
    if (!root.inode) {
        goto err;
    }

    return 0;

  err:
    close(fs.fd);
    return ret;
}

static void kfs_umount()
{
    int ret;
    ret = kfs_sync_fs(&fs);
    if (ret) {
        kwarn("Sync filesystem failed\n");
    }
    ret = close(fs.fd);
    if (ret) {
        kwarn("Sync filesystem failed\n");
    }
}

int main(int argc, char *argv[])
{
    int ret;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    openlog("KFS", LOG_PID|LOG_CONS, LOG_USER);
    setlogmask(LOG_UPTO(LOG_DEBUG));

    if (fuse_opt_parse(&args, &kfs_param, kfs_opts, kfs_process_arg)) {
        printf("failed to parse option\n");
        return 1;
    }

    if (!kfs_param.filename) {
        printf("need to give the filename\n");
        return 1;
    }
    kfs_log_level = kfs_param.logLevel;
    kdebug(LOG_OBJECT, "filename: %s\n", kfs_param.filename);
    kdebug(LOG_OBJECT, "logLevel: %d\n", kfs_param.logLevel);

    memset(&fs, 0, sizeof(fs));

    ret = kfs_mount();
    if (ret < 0) {
        goto no_mount;
    }

    ret = fuse_main(args.argc, args.argv, &kfs_operations, NULL);
    if (ret < 0) {
        kerr("Mount real fs error %d\n", ret);
    }

    kfs_umount();
  no_mount:
    return ret;
}
