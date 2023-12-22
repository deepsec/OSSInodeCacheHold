#ifndef __DS_SYSCALL_H__
#define __DS_SYSCALL_H__

//#define _POSIX_C_SOURCE 199309L
#include <time.h>

#define SYSCALL_SLEEP_MILLISECONDS      50
#define SYSCALL_CALL_MAX_RETRY_TIMES    10

void mkalldir(char *dir, mode_t mode);

int syscall_msleep(long ms);
int syscall_usleep(long us);
int syscall_sleep(long s);
int sleep_open(const char *pathname, int flags, mode_t mode);
int sleep_rename(const char *oldpath, const char *newpath);
int fs_info(const char *fs, long *ino_free, long *ino_total, long *blk_free, long *blk_total, long *blk_size);

#endif

