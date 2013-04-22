/* vim: set expandtab ts=4 sw=4:                               */

/*-===========================================================-*/
/*  Author:                                                    */
/*        KevinKW                                              */
/*-===========================================================-*/

#include <kfs.h>

#if 0
#ifdef USE_SYSLOG
#undef USE_SYSLOG
#endif
#endif

static char *pname = NULL;

void mkfs_version()
{
    printf("%s version: %s_%s\n", pname, KFS_NAME, KFS_VERSION);
}

int mkfs_usage()
{
    printf("usage: %s\n", pname);
    printf("options:\n");
    printf("    -f|--file filename     file path to create the kfs on\n");
    printf("    -i|--inode_bg_size     inode group size (default %dM)\n", getm(DEFAULT_IBG_SIZE));
    printf("    -b|--block_bg_size     block group size (default %dM)\n", getm(DEFAULT_BBG_SIZE));
    return 1;
}

static struct option kfs_mkfs_opts[] = {
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'v' },
    { "file", required_argument, NULL, 'f' },
    { "inode_bg_size", required_argument, NULL, 'i' },
    { "block_bg_size", required_argument, NULL, 'b' },
    { NULL, no_argument, NULL, 0 }
};

static char file[256];
static u32 inode_bg_size = DEFAULT_IBG_SIZE;
static u32 block_bg_size = DEFAULT_BBG_SIZE;
static struct kfs fs;

int main(int argc, char **argv)
{
    char *p;
    int i, ret;

    pname = argv[0];
    if ((p = strrchr(pname, '/')) != NULL)
        pname = p+1;

    if (argc < 2) {
        return mkfs_usage();
    }

    for (i = 0; i < argc; i++) {
        if ((strnmatch(argv[i], "-h")) ||
                (strnmatch(argv[i], "--help"))) {
            mkfs_usage();
            return 0;
        }
        if ((strnmatch(argv[i], "-v")) ||
                (strnmatch(argv[i], "--version"))) {
            mkfs_version();
            return 0;
        }
    }

    char c;
    while ((c = getopt_long(argc, argv, "f:i:b:",
                    kfs_mkfs_opts, NULL)) != -1) {
        switch (c) {
            case 'f':
                snprintf(file, 256, "%s", optarg);
                break;
            case 'i':
                inode_bg_size = atoi(optarg);
                inode_bg_size <<= 20;
                break;
            case 'b':
                block_bg_size = atoi(optarg);
                block_bg_size <<= 20;
                break;
            case 'h':
                mkfs_usage();
                return 0;
            case 'v':
                mkfs_version();
                return 0;
            default:
                return mkfs_usage();
        }
    }

    argc -= optind;
    if (argc) {
        if (argc == 1) {
            snprintf(file, 256, "%s", argv[optind]);
        } else {
            return mkfs_usage();
        }
    }

    kdebug(LOG_MKFS, "file %s ibg_size %u dbg_size %u\n",
            file, inode_bg_size, block_bg_size);

    if ((inode_bg_size < MIN_IBG_SIZE) || (inode_bg_size > MAX_IBG_SIZE)) {
        kerr("Invalid inode block group size\n");
        return 1;
    }

    if ((block_bg_size < MIN_BBG_SIZE) || (block_bg_size > MAX_BBG_SIZE)) {
        kerr("Invalid block block group size\n");
        return 1;
    }

    kfs_init(&fs);

    fs.fd = open(file, O_CREAT|O_EXCL|O_RDWR|O_NOFOLLOW, 0644);
    //fs.fd = open(file, O_CREAT|O_RDWR|O_TRUNC|O_NOFOLLOW, 0644);
    if (fs.fd < 0) {
        kerr("Open file %s failed: %s\n", file, strerror(errno));
        return 1;
    }

    ret = ftruncate(fs.fd, KFS_SB_SIZE);
    if (ret < 0) {
        kerr("Generate superblock failed: %s\n",
                strerror(errno));
        goto err;
    }

    /* Generate sb */
    fs.sb.magic = KFS_SB_MAGIC;
    fs.sb.version = KFS_SB_VERSION;
    fs.sb.ibg_size = inode_bg_size;
    fs.sb.dbg_size = block_bg_size;

    ret = pwrite(fs.fd, &fs.sb, sizeof(fs.sb), 0);
    if (ret != sizeof(fs.sb)) {
        kerr("Write superblock failed %s\n",
                strerror(errno));
        goto err;
    }
    fs.filesize = KFS_SB_SIZE;
    fs.inode_per_bg = kfs_ibg_size(&fs) / KFS_INODE_SIZE;
    fs.block_per_bg = kfs_dbg_size(&fs) / KFS_BLOCK_SIZE;

#if 0
    ret = kfs_extend_bg(&fs, KFS_BG_INODE);
    if (ret) {
        kerr("Write the first inode group failed\n");
        goto err;
    }
#endif

    struct kfs_inode *inode;
    ret = kfs_alloc_inode(&fs, &inode);
    if (ret) {
        kerr("Generate root inode failed\n");
        goto err;
    }

    kfs_lock_inode(inode);
    inode->node.uid = KFS_DEFAULT_ROOT_UID;
    inode->node.gid = KFS_DEFAULT_ROOT_GID;
    inode->node.mode = KFS_DEFAULT_ROOT_MODE|S_IFDIR;
    inode->node.nlink = 1;
    inode->node.btime = time(NULL);
    inode->node.ctime = inode->node.atime = inode->node.mtime = inode->node.btime;
    mark_inode_dirty(inode, 1);
    kfs_unlock_inode(inode);

    ret = kfs_sync_fs(&fs);
    if (ret) {
        goto err;
    }

    return 0;

  err:
    close(fs.fd);
    remove(file);
    return 1;
}
