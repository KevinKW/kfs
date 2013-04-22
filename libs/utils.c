/* vim: set expandtab ts=4 sw=4:                               */

/*-===========================================================-*/
/*  Author:                                                    */
/*        KevinKW                                              */
/*-===========================================================-*/

#include <kfs.h>
#include <string.h>

int kfs_log_modules = DEFAULT_LOG_MODULES;
int kfs_log_level = DEFAULT_LOG_LEVEL;

int strnmatch(char *str1, char *str2)
{
    int size = strlen(str2);
    if (strlen(str1) != size)
        return 0;

    return (strncmp(str1, str2, size) == 0);
}

int getk(int size)
{
    return size>>10;
}

int getm(int size)
{                                                                                                                                  
    return size>>20;
}

void kfs_set_bit(u32 nr, void *addr, pthread_mutex_t *lock)
{
    u8 *byte;
    u32 index;
    u8 shift;
    u8 mask = 1;

    byte = (u8*)addr;
    index = nr >> 3;
    shift = nr & 0x7;
    byte += index;
    mask <<= shift;

    if (lock)
        pthread_mutex_lock(lock);
    *byte |= mask;
    if (lock)
        pthread_mutex_unlock(lock);
}

void kfs_clear_bit(u32 nr, void *addr, pthread_mutex_t *lock)
{
    u8 *byte;
    u32 index;
    u8 shift;
    u8 mask = 1;

    byte = (u8*)addr;
    index = nr >> 3;
    shift = nr & 0x7;
    byte += index;
    mask <<= shift;

    if (lock)
        pthread_mutex_lock(lock);
    *byte &= ~mask;
    if (lock)
        pthread_mutex_unlock(lock);
}

int kfs_test_bit(u32 nr, void *addr, pthread_mutex_t *lock)
{
    u8 *byte;
    u32 index;
    u8 shift;
    u8 mask = 1;
    int ret;

    byte = (u8*)addr;
    index = nr >> 3;
    shift = nr & 0x7;
    byte += index;
    mask <<= shift;

    if (lock)
        pthread_mutex_lock(lock);
    ret = (*byte & mask);
    if (lock)
        pthread_mutex_unlock(lock);

    return ret;
}
