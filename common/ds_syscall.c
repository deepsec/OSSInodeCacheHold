#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <sys/statvfs.h>
#include "ds_common.h"
#include "ds_err.h"
#include "ds_syscall.h"
#include <time.h>

void mkalldir(char *dir, mode_t mode)
{
	char *p;
	char all_dir[PATH_MAX] = {0};
	int i;
	int retry_times = 0;

	if (dir == NULL) return;

	for (i = 0, p = dir; i <= strlen(dir); i++) {
		if (i == 0 && *p == '/') {	/* skip root directory */
			continue;
		}
		if (p[i] != '/' && p[i] != '\0') {
			continue;
		}
		strncpy(all_dir, dir, i);		
RETRY_MKDIR:
		if (mkdir(all_dir, mode) < 0) {
			if (errno == ENOSPC || errno == EBUSY || errno == EAGAIN || errno == EINTR) {
					syscall_sleep(SYSCALL_SLEEP_MILLISECONDS);
					if (retry_times++ < SYSCALL_CALL_MAX_RETRY_TIMES) {
						goto RETRY_MKDIR;
					}
			}
			if (errno != EEXIST) {	
				err_msg("mkdir() syscall reached max retry: %d", SYSCALL_CALL_MAX_RETRY_TIMES);				
				err_sys("mkdir(%s, %o) error", all_dir, mode);
			}
		}
	}
	return;
}

int sleep_open(const char *pathname, int flags, mode_t mode)
{
	int fd;
	int retry_times = 0;

RETRY_OPEN:
	if ((fd = open(pathname, flags, mode)) < 0) {
		if (errno == ENOSPC || errno == EBUSY || errno == EAGAIN || errno == EINTR) {
			syscall_sleep(SYSCALL_SLEEP_MILLISECONDS);
			if (retry_times++ < SYSCALL_CALL_MAX_RETRY_TIMES) {
				goto RETRY_OPEN;
			}
		}
		err_msg("open() syscall reached max retry: %d", SYSCALL_CALL_MAX_RETRY_TIMES);
		err_sys("open(%s, %o, %o) error", pathname, flags, mode);
	}
	return fd;
}	

int sleep_rename(const char *oldpath, const char *newpath)
{
	int ret;
	int retry_times = 0;

RETRY_RENAME:
	if ((ret = rename(oldpath, newpath)) < 0) {
		if (errno == ENOSPC || errno == EBUSY || errno == EAGAIN || errno == EINTR) {
			syscall_sleep(SYSCALL_SLEEP_MILLISECONDS);
			if (retry_times++ < SYSCALL_CALL_MAX_RETRY_TIMES) {
				goto RETRY_RENAME;
			}
		}
		err_msg("rename() syscall reached max retry: %d", SYSCALL_CALL_MAX_RETRY_TIMES);
		err_sys("rename(%s, %s) error", oldpath, newpath);
	}
	return ret;
}


int syscall_msleep(long ms)
{
	struct timespec ts;
	int ret;

	ts.tv_sec = 0;
	ts.tv_nsec = (ms % 1000) * 1000000;
	ret = nanosleep(&ts, NULL);
	if (ret < 0) {
		err_sys("nanosleep error");
	}
	return ret;
}

int syscall_usleep(long us)
{
	struct timespec ts;
	int ret;

	ts.tv_sec = 0;
	ts.tv_nsec = (us % 1000000) * 1000;
	ret = nanosleep(&ts, NULL);
	if (ret < 0) {
		err_sys("nanosleep error");
	}
	return ret;
}

int syscall_sleep(long s)
{
	struct timespec ts;
	int ret;

	ts.tv_sec = s % (86400 * 365);
	ts.tv_nsec = 0;
	ret = nanosleep(&ts, NULL);
	if (ret < 0) {
		err_sys("nanosleep error");
	}
	return ret;
}



int fs_info(const char *fs, long *ino_free, long *ino_total, long *blk_free, long *blk_total, long *blk_size)
{
	struct statvfs sv;

	if (fs == NULL) return -1;
    if (statvfs(fs, &sv) < 0) {
        //err_ret("statvfs [%s] error", fs);
		return -1;
    }

	if (ino_free)	*ino_free = sv.f_ffree;
	if (ino_total)	*ino_total = sv.f_files;
	if (blk_free)	*blk_free = sv.f_bfree;
	if (blk_total)	*blk_total = sv.f_blocks;
	if (blk_size)	*blk_size = sv.f_bsize;

    return 0;
}
