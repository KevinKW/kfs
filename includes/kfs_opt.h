/* vim: set expandtab ts=4 sw=4:                               */

/*-===========================================================-*/
/*  Author:                                                    */
/*        KevinKW                                              */
/*-===========================================================-*/

/*
 * Define the common structures that are used for KFS mount
 *
 */
#ifndef __KFS_OPT_H__
#define __KFS_OPT_H__

#define KFS_MACRO_TO_STR_HELPER(s) #s
#define KFS_MACRO_TO_STR(s) KFS_MACRO_TO_STR_HELPER(s)

/*
 * v0.x kfs based on fuse
 * v1.x The first release candidate
 * v2.x kfs based on kernel and tests
 * v3.x Final releases
 */
#define KFS_VERSION_MAJOR   0.2
#define KFS_VERSION_MINOR   0
#define KFS_VERSION_RELEASE 0

#if (KFS_VERSION_RELEASE == 0)
#define KFS_VERSION KFS_MACRO_TO_STR(KFS_VERSION_MAJOR) "." KFS_MACRO_TO_STR(KFS_VERSION_MINOR)
#else
#define KFS_VERSION KFS_MACRO_TO_STR(KFS_VERSION_MAJOR) "." KFS_MACRO_TO_STR(KFS_VERSION_MINOR) "-" KFS_MACRO_TO_STR(KFS_VERSION_RELEASE)
#endif


/* Compile options */
#if 0
#define KFS_RELEASE_BUILD
#endif

#if 1
#define USE_SYSLOG
#endif

#if 1
#define KFS_HIGH_PERF
#endif

#if 0
#define KFS_KLOG
#endif

#ifndef KFS_RELEASE_BUILD
#if 1
#define KFS_DEBUG
#endif
#if 1
#define KFS_ENABLE_ASSERT
#endif
#if 1
#define PRINT_FUNCTION
#endif

#if 1
#define KFS_DEBUG_MEM
#endif
#if 0
#define KFS_DEBUG_MEM2
#endif

#if 1
#define KFS_DEBUG_FILE
#endif

#if 1
#define KFS_DEBUG_SERVER_LIST
#endif

#if 1
#define KFS_DEBUG_PAGELOCK
#endif

#if 0
#define KFS_DEBUG_FS_LOCK
#endif

#if 0
#define KFS_DEBUG_SERVER_LOCK
#endif

#if 1
#define KFS_DEBUG_IOREQ
#endif

#if 1
#define KFS_CONN_LATENCY
#endif

#if 0
#define KFS_TEST_ICMP
#endif
#endif /* KFS_RELEASE_BUILD */

#ifndef KFS_DEBUG
#if 1
#define UNTRUSTED_SERVER
#endif
#endif

#if 0
#define KFS_NO_CRYPTO
#endif

#if 0
#define KFS_FS_THREADPOOL
#endif

#if 1
#define KFS_FS_HAD
#endif

#if 1
#if (PAGE_CACHE_SIZE == 4096)
#define KFS_PAGE_PERF
#endif
#endif

#if 1
#define KFS_USE_MEMPOOL
#endif

#if 1
#define KFS_MWRITE_ENABLE
#endif

#if 0
#define KFS_SILLY_RENAME
#endif

#if 0
#define KFS_DEBUG_READ
#endif

#if 0
#define KFS_DEBUG_WRITE
#endif

#if 1
#define KFS_PERCPU
#endif

// Save config to a file
//#define SAVE_CONFIG
#if 0
#define SHOW_DEFAULT_MOUNT_OPTION
#endif

#if 1
#define KFS_ENABLE_FORCE_UMOUNT
#endif

#if 0
#define KFS_SUPPORT_HARDLINK
#endif

#if 0
#define KFS_SUPPORT_MKNOD
#endif

#if 0
#define KFS_SUPPORT_GETATTR
#endif

#if 1
#define KFS_SUPPORT_SETATTR
#endif

#if 1
#define KFS_SUPPORT_RDIRPLUS
#endif

#if 0
#define KFS_HAVE_SETXATTR
#endif

#if 0
#define KFS_SUPPORT_PREALLOC
#endif

#if 0
#define KFS_SUPPORT_OPEN
#endif

#if 0
#define KFS_SUPPORT_CLOSE
#endif

#if 0
#define KFS_SUPPORT_COMMIT
#endif

#if 0
#define KFS_SUPPORT_LOCK
#endif

#if 0
#define KFS_ASYNC_WAIT
#endif

#if 0
#define KFS_SUPPORT_READ_FRIEND
#endif

/* Supported Protocols */
#if 0
#define KFS_SUPPORT_WRITE_FRIEND
#endif

#if !defined(KFS_SINGLE_MOUNT_MODE) || defined(KFS_LOCAL_MOUNT_ONLY)
#if defined(KFS_SUPPORT_READ_FRIEND) || defined(KFS_SUPPORT_WRITE_FRIEND)
#define KFS_SUPPORT_FRIEND
#endif
#endif

/* Supported Protocols */
#if 1
#define KFS_REST_PROTO
#endif

#if 1
#define KFS_LOONGDISK_PROTO
#endif

#if !defined(KFS_SINGLE_MOUNT_MODE) || defined(KFS_LOCAL_MOUNT_ONLY)
#if 0
#define KFS_SUPPORT_SHARE
#endif
#endif

#if 0
#define KFS_PERF_TEST
#endif

#ifndef KFS_HIGH_PERF
#if 0
#define KFS_MOUNT_DEFAULT_QUOTA
#endif
#endif

#ifndef KFS_HIGH_PERF
#if 0
#define KFS_MOUNT_DEFAULT_CRYPTO
#endif
#endif

#if 0
#define KFS_IGNCASE_OPTION
#endif

#if 1
#define KFS_MOUNT_DEFAULT_IGNCASE
#endif

#if defined(KFS_GLOBAL_MOUNT_ONLY) || defined(KFS_MOUNT_DEFAULT_GLOBAL)
#define DEFAULT_MOUNT_MODE KFS_MOUNT_GLOBAL
#else
#define DEFAULT_MOUNT_MODE 0
#endif

#ifdef KFS_MOUNT_DEFAULT_QUOTA
#define DEFAULT_QUOTA_MODE KFS_MOUNT_QUOTA
#else
#define DEFAULT_QUOTA_MODE 0
#endif

#ifdef KFS_MOUNT_DEFAULT_CRYPTO
#define DEFAULT_CRYPTO_MODE KFS_MOUNT_CRYPTO
#else
#define DEFAULT_CRYPTO_MODE 0
#endif

#ifdef KFS_MOUNT_DEFAULT_IGNCASE
#define DEFAULT_IGNCASE_MODE KFS_MOUNT_IGNCASE
#else
#define DEFAULT_IGNCASE_MODE 0
#endif

#if defined(KFS_LOCAL_MOUNT_ONLY) || defined(KFS_MOUNT_DEFAULT_LOCAL)
#define DEFAULT_RDIR_MODE KFS_MOUNT_RDIRPLUS
#else
#define DEFAULT_RDIR_MODE 0
#endif

#ifndef PAGE_CACHE_SIZE
#ifdef LOONGSON
#define PAGE_CACHE_SIZE (16 << 10) // 16K
#else
#define PAGE_CACHE_SIZE (4 << 10) // 4K
#endif
#endif

#ifndef PAGE_CACHE_SHIFT
#ifdef LOONGSON
#define PAGE_CACHE_SHIFT 14
#else
#define PAGE_CACHE_SHIFT 12
#endif
#endif

#define DEFAULT_MOUNT_FLAGS \
    DEFAULT_MOUNT_MODE|     \
    DEFAULT_RDIR_MODE|      \
    DEFAULT_QUOTA_MODE|     \
    DEFAULT_CRYPTO_MODE|    \
    DEFAULT_IGNCASE_MODE

#define PACKAGE_SIZE        (2<<20)      // 2M
#ifdef KFS_HIGH_PERF
#define DEFAULT_RASIZE      (1<<20)      // 1M
#else
#define DEFAULT_RASIZE      (2<<20)      // 2M
#endif
#define MIN_RASIZE          (128<<10)    // 128K
#define MAX_RASIZE          (16<<20)     // 16M

#ifdef KFS_HIGH_PERF
#define DEFAULT_RSIZE       (64<<10)     // 64K
#else
#define DEFAULT_RSIZE       (32<<10)     // 32K
#endif
#define MIN_RSIZE           (1 << PAGE_CACHE_SHIFT)
#define MAX_RSIZE           PACKAGE_SIZE

#ifdef KFS_HIGH_PERF
#define DEFAULT_WSIZE       (128<<10)     // 128K
#else
#define DEFAULT_WSIZE       (64<<10)     // 64K
#endif
#define MIN_WSIZE           (1 << PAGE_CACHE_SHIFT)
#define MAX_WSIZE           PACKAGE_SIZE

#define DEFAULT_LANG        0

#define DEFAULT_TIMEOUT     10
#define MAX_TIMEOUT         999
#define DEFAULT_ACTIMEOUT   3
#define MAX_ACTIMEOUT       3600 // seconds of one hour
#define DEFAULT_NDTIMEOUT   3
#define MAX_NDTIMEOUT       3600 // seconds of one hour
#define DEFAULT_RETRY       3
#define MAX_RETRY           99
#define DEFAULT_LATENCY     0
#define MAX_LATENCY         999999
#define DEFAULT_FORCE       0
#define DEFAULT_CRYPTO      0
#ifdef KFS_HIGH_PERF
#define DEFAULT_CONN        16
#else
#define DEFAULT_CONN        8
#endif
#define MAX_CONN            64

#define DEFAULT_KEEPALIVE   65
#define DEFAULT_MAX_REQ     50000000

#ifdef KFS_DEBUG
#define DEFAULT_LOG_MODULES 0xFFFF
#define DEFAULT_LOG_LEVEL   7
#else
#define DEFAULT_LOG_MODULES 0x0001
#define DEFAULT_LOG_LEVEL   6
#endif

#define INVALID_OPT -1

#ifdef KFS_HIGH_PERF
#define DEFAULT_THREAD_NUM 16
#else
#define DEFAULT_THREAD_NUM 8
#endif
#define MAX_THREAD_NUM 64
#define MIN_THREAD_NUM 1

#define DEFAULT_NET_THREAD_NUM 16
#define MAX_NET_THREAD_NUM 64
#define MIN_NET_THREAD_NUM 1

#ifdef KFS_HIGH_PERF
#define DEFAULT_SERVER_NUM 8
#else
#define DEFAULT_SERVER_NUM 4
#endif
#define MAX_SERVER_NUM 32
#define MIN_SERVER_NUM 1

#ifdef KFS_HIGH_PERF
#define DEFAULT_RIO_MAX     16
#define DEFAULT_WIO_MAX     16
#else
#define DEFAULT_RIO_MAX     8
#define DEFAULT_WIO_MAX     8
#endif
#define MAX_IO_MAX 512
#define MIN_IO_MAX 1

#define DEFAULT_HA_INTERVAL 30
#define MAX_KFSHAD_INTERVAL 600
#define MIN_KFSHAD_INTERVAL 1

#define DEFAULT_LOG_BUFFER_SIZE 4096*16
#define DEFAULT_LOG_FILE "/var/log/kfs.log"

#define OPTION_LEN 256
#define BUFSIZE 256
#define TIME_BUFLEN 32

// KFS Stats related definations
#if 1
#define KFS_STATS
#endif
#ifdef KFS_STATS
#if 1
#define KFS_GLOBAL_STATS
#endif
#if 1
#define KFS_FS_STATS
#endif
#if 1
#define KFS_USER_STATS
#endif
#if 0
#define KFS_SERVER_STATS
#endif
#endif

#ifndef KFS_KERNEL
typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned char u8;
#endif

#define S_IRWXUGO   (S_IRWXU|S_IRWXG|S_IRWXO)

#define FILE_READ_FLAGS     (S_IRUSR|S_IRGRP|S_IROTH)
#define FILE_WRITE_FLAGS    (S_IWUSR|S_IRGRP|S_IROTH)
#define FILE_EXEC_FLAGS     (S_IXUSR|S_IXGRP|S_IXOTH)
#define KFS_DEFAULT_FILE_UID 0
#define KFS_DEFAULT_FILE_GID 0
#define KFS_DEFAULT_FILE_MODE (FILE_READ_FLAGS|FILE_WRITE_FLAGS|FILE_EXEC_FLAGS)
#define KFS_DEFAULT_LINK_MODE S_IRWXU|S_IRWXG|S_IRWXO // i.e. S_IRWXUGO

#define KFS_DEFAULT_ROOT_UID 0
#define KFS_DEFAULT_ROOT_GID 0
#define KFS_DEFAULT_ROOT_MODE S_IRWXUGO

#define KFS_NAME "kfs"

#define KFS_SB_MAGIC       0xABCDABCD
#define KFS_SB_VERSION     1
#define KFS_INODE_SIZE  256
#define KFS_BLOCK_SIZE  4096
#define KFS_BGD_SIZE    4096
#define KFS_SB_SIZE     8192
#define KFS_BITMAP_SIZE 4096
#define KFS_BG_META_SIZE (KFS_BGD_SIZE+KFS_BITMAP_SIZE)
#define KFS_DB_NUM      15
#define KFS_BITMAP_BITS (KFS_BLOCK_SIZE * 8) /* Only 1 block for bitmap */
#define KFS_BLOCK_SHIFT 12
#define KFS_INODE_SHIFT 8

#define KFS_FILENAME_LEN 256
#define KFS_PATH_LEN     1024

#define DEFAULT_IBG_SIZE (1<<20)
#define DEFAULT_BBG_SIZE (64<<20)
#define MIN_IBG_SIZE     (1<<20)
#define MAX_IBG_SIZE     (KFS_BITMAP_BITS * KFS_INODE_SIZE)
#define MIN_BBG_SIZE     (1<<20)
#define MAX_BBG_SIZE     (KFS_BITMAP_BITS * KFS_BLOCK_SIZE)

/* Using this file to help modify the build options as wanted */
//#include "kfs_build_helper.h"
#endif //__KFS_OPT_H__
