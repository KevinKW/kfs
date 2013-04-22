/* vim: set expandtab ts=4 sw=4:                               */

/*-===========================================================-*/
/*  Author:                                                    */
/*        KevinKW                                              */
/*-===========================================================-*/

/*
 * Define the libs interfaces that are used by KFS.
 *
 */
#ifndef __KFS_LIBS_H__
#define __KFS_LIBS_H__

struct kfs;
struct kfs_bg;
struct kfs_inode;
struct kfs_dentry;

#ifndef KFS_KERNEL
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
#endif

#include <kfs_opt.h>
//#include <kfs_mem.h>

#ifdef KFS_ENABLE_ASSERT
#ifdef KFS_KERNEL
#define KFS_ASSERT(condition) BUG_ON(!(condition))
#else
#include <assert.h>
#define KFS_ASSERT(condition) assert(condition)
#endif
#else
#define KFS_ASSERT(condition)
#endif

#ifndef KFS_KERNEL
#define likely(condition) (condition)
#define unlikely(condition) (condition)
#endif

#ifdef KFS_KERNEL
#define real_print(logLevel, ...) printk(__VA_ARGS__)
#define LOG_EMERG   0   /* system is unusable */
#define LOG_ALERT   1   /* action must be taken immediately */
#define LOG_CRIT    2   /* critical conditions */
#define LOG_ERR     3   /* error conditions */
#define LOG_WARNING 4   /* warning conditions */
#define LOG_NOTICE  5   /* normal but significant condition */
#define LOG_INFO    6   /* informational */
#define LOG_DEBUG   7   /* debug-level messages */
#define LOG_DEBUG2  8   /* debug-level messages */
#define LOG_DEBUG3  9   /* debug-level messages */
#else
#include <syslog.h>
#define LOG_DEBUG2  8   /* debug-level messages */
#define LOG_DEBUG3  9   /* debug-level messages */
#define KERN_ERR "<Error>"
#define KERN_WARNING "<Warning>"
#define KERN_INFO "<Info>"
#define KERN_DEBUG "<Debug>"
#define jiffies time(NULL)
#ifdef USE_SYSLOG
#define real_print(logLevel, ...) syslog(logLevel, __VA_ARGS__)
#else
#define real_print(logLevel, ...) printf(__VA_ARGS__)
#endif
#endif

extern int klog(int logModules, int logLevel, const char *fmt, ...);
extern int config_klog(char *filename, int buffer_size,
        int log_to_file_level, int log_to_printk_level, int threashold);
extern int init_klog(void);
extern void destroy_klog(void);

/* Define log functions */
#define LOG_ALWAYS      0x0001
#define LOG_MKFS        0x0002
#define LOG_THREADS     0x0004
#define LOG_VFS         0x0008
#define LOG_LOCKS       0x0010
#define LOG_PROTOCOL    0x0020
#define LOG_IO          0x0040
#define LOG_PROC        0x0080
#define LOG_OBJECT      0x0100
#define LOG_LIBS        0x0200
#define LOG_PATH        0x0400
#define LOG_MEMORY      0x2000
#define LOG_I18N        0x4000
#define LOG_ALL         0xFFFF

#ifndef KFS_RELEASE_BUILD
extern int kfs_log_modules;
extern int kfs_log_level;
#endif

#ifdef KFS_KLOG
#define msg(logModule, logLevel, ...) klog(logModule, logLevel, __VA_ARGS__)
#else
#ifdef KFS_RELEASE_BUILD
#define msg(logModule, logLevel, ...) real_print(logLevel, __VA_ARGS__)
#else
#define msg(logModule, logLevel, ...) \
    if (unlikely(((kfs_log_modules|LOG_ALWAYS) & logModule) && \
                (logLevel <= kfs_log_level))) { \
        real_print(logLevel, __VA_ARGS__); \
    }
#endif
#endif

#ifdef KFS_DEBUG
#ifndef PRINT_FUNCTION
#define PRINT_FUNCTION
#endif
#endif

#ifdef PRINT_FUNCTION
#define kerr(fmt, args...) msg(LOG_ALWAYS, LOG_ERR, KERN_ERR "KFS: (%s) " fmt, __FUNCTION__, ## args)
#define kwarn(fmt, args...) msg(LOG_ALWAYS, LOG_WARNING, KERN_WARNING "KFS: (%s) " fmt, __FUNCTION__, ## args)
#define kinfo(fmt, args...) msg(LOG_ALWAYS, LOG_INFO, KERN_INFO "KFS: (%s) " fmt, __FUNCTION__, ## args)
#else
#define kerr(fmt, args...) msg(LOG_ALWAYS, LOG_ERR, KERN_ERR "KFS: " fmt, ## args)
#define kwarn(fmt, args...) msg(LOG_ALWAYS, LOG_WARNING, KERN_WARNING "KFS: " fmt, ## args)
#define kinfo(fmt, args...) msg(LOG_ALWAYS, LOG_INFO, KERN_INFO "KFS: " fmt, ## args)
#endif

#ifndef KFS_RELEASE_BUILD
#define kdebug(logModule, fmt, args...) msg(logModule, LOG_DEBUG, KERN_DEBUG "KFS: (%lu:%s) " fmt, jiffies, __FUNCTION__, ## args)

//#define kdebug2(logModule, fmt, args...)
#define kdebug2(logModule, fmt, args...) msg(logModule, LOG_DEBUG2, KERN_DEBUG "KFS: (%lu:%s) " fmt, jiffies, __FUNCTION__, ## args)

//#define kdebug3(logModule, fmt, args...)
#define kdebug3(logModule, fmt, args...) msg(logModule, LOG_DEBUG3, KERN_DEBUG "KFS: (%lu:%s) " fmt, jiffies, __FUNCTION__, ## args)
#else
#define kdebug(logModule, fmt, args...)
#define kdebug2(logModule, fmt, args...)
#define kdebug3(logModule, fmt, args...)
#endif

#ifndef KFS_KERNEL
#define kfs_alloc(mt, s) malloc(s)
#define kfs_free(mt, p) free(p)
#endif

#define do_retry(i, retry) for(i = 0; ((!retry) || (i < retry)); i++)

static inline void *err_cast(const void *ptr)
{
    return (void *) ptr;
}

#ifndef KFS_KERNEL
/* Add list_head implementation */
#define container_of(ptr, type, member) ({                              \
                        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                        (type *)( (char *)__mptr - offsetof(type,member) );})

#define list_entry(ptr, type, member)           \
        container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
        list_entry((ptr)->next, type, member)

struct list_head {
	struct list_head *next;
	struct list_head *prev;
};

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

static inline void __list_add(struct list_head *new, struct list_head *prev,
		     struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

static inline void list_del(struct list_head *entry)
{
	struct list_head *prev = entry->prev;
	struct list_head *next = entry->next;

	next->prev = prev;
	prev->next = next;
}

static inline void list_del_init(struct list_head *entry)
{
    list_del(entry);
    INIT_LIST_HEAD(entry);
}

static inline void __list_splice(const struct list_head *list,
        struct list_head *prev,
        struct list_head *next)
{
    struct list_head *first = list->next;
    struct list_head *last = list->prev;

    first->prev = prev;
    prev->next = first;

    last->next = next;
    next->prev = last;
}


static inline void list_splice(const struct list_head *list,
        struct list_head *head)
{
    if (!list_empty(list))
        __list_splice(list, head, head->next);
}

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
            pos = n, n = pos->next)

#define list_for_each_entry(pos, head, member)              \
    for (pos = list_entry((head)->next, typeof(*pos), member);  \
            &pos->member != (head);    \
            pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_reverse(pos, head, member)          \
    for (pos = list_entry((head)->prev, typeof(*pos), member);  \
            &pos->member != (head);    \
            pos = list_entry(pos->member.prev, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)          \
    for (pos = list_entry((head)->next, typeof(*pos), member),  \
            n = list_entry(pos->member.next, typeof(*pos), member); \
            &pos->member != (head);                    \
            pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_for_each_entry_safe_reverse(pos, n, head, member)      \
    for (pos = list_entry((head)->prev, typeof(*pos), member),  \
            n = list_entry(pos->member.prev, typeof(*pos), member); \
            &pos->member != (head);                    \
            pos = n, n = list_entry(n->member.prev, typeof(*n), member))
#endif


extern int strnmatch(char *str1, char *str2);
extern int getk(int size);
extern int getm(int size);

extern void kfs_init(struct kfs *fs);
extern int kfs_build_bgs(struct kfs *fs);
extern void kfs_init_bg(struct kfs *fs, struct kfs_bg *bg, u64 id, u32 type, u64 offset);
extern struct kfs_bg *kfs_get_ibg(struct kfs *fs, u64 ino);
extern void lock_for_extend_fs(struct kfs *fs);
extern void unlock_for_extend_fs(struct kfs *fs);
extern void lock_bgs(struct kfs *fs, u32 type);
extern void unlock_bgs(struct kfs *fs, u32 type);
extern u64 bg_offset(struct kfs_bg *bg);
extern void lock_bg(struct kfs_bg *bg);
extern void unlock_bg(struct kfs_bg *bg);
extern void make_fs_ok(struct kfs *fs, int locked);
extern void mark_fs_err(struct kfs *fs, int locked);
extern void mark_fs_dirty(struct kfs *fs, int locked);
extern void kfs_set_bit(u32 nr, void *addr, pthread_mutex_t *lock);
extern void kfs_clear_bit(u32 nr, void *addr, pthread_mutex_t *lock);
extern int kfs_test_bit(u32 nr, void *addr, pthread_mutex_t *lock);
extern int kfs_extend_bg(struct kfs *fs, u32 type);
extern struct kfs_inode *kfs_get_inode(struct kfs *fs, u64 ino);
extern int kfs_alloc_inode(struct kfs *fs, struct kfs_inode **inodep);
extern int kfs_alloc_inode_bg(struct kfs_bg *ibg, u64 *ino);
extern int kfs_read_sb(struct kfs *fs);
extern int kfs_sync_fs(struct kfs *fs);
extern int kfs_sync_sb(struct kfs *fs);
extern int kfs_sync_bgs(struct kfs *fs, u32 type);
extern void kfs_lock_inode(struct kfs_inode *inode);
extern void kfs_unlock_inode(struct kfs_inode *inode);
extern void mark_inode_dirty(struct kfs_inode *inode, int locked);
extern int kfs_sync_inode(struct kfs_inode *inode, int locked);
extern u64 inode_offset(struct kfs_inode *inode);
extern void kfs_init_dentry(struct kfs_dentry *dentry, char *name, int namelen);
extern void kfs_ihash_insert(struct kfs_bg *ibg, struct kfs_inode *inode, int locked);
extern void kfs_init_inode(struct kfs_inode *inode);
extern void kfs_inc_iused(struct kfs *fs);
#endif //__KFS_LIBS_H__
