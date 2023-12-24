#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <openssl/md5.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/xattr.h>
#include "ds_err.h"
#include "ds_common.h"
#include "ds_syscall.h"
#include "OSSInodeCacheHold.h"

void recursive_dir(const char *dir, struct pthread_dir_info *pdi, long dir_depth)
{
	DIR *dirp;
	struct dirent *dp;
	struct stat stbuf;
	char tmp[PATH_MAX + 1] = { 0 };
	char vd_orig[64] = { 0 }, vd_new[64] = { 0 }, vf_orig[64] = { 0 }, vf_new[64] = { 0 }, vl_orig[64] = { 0 }, vl_new[64] = { 0 };
	char date_buf[32] = { 0 };
	
	//dump_pdi_info(pdi);
	//return;

	if (dir == NULL)
		return;
	if ((dirp = opendir(dir)) == NULL) {
		//err_msg("opendir(%s) error", dir);
		return;
	}
	while ((dp = readdir(dirp)) != NULL) {
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
			continue;
		}
		snprintf(tmp, sizeof(tmp), "%s/%s", dir, dp->d_name);
		if (pdi->all_delay == 1) {
			syscall_usleep(pdi->usecs);
		}
		/* the last level, no lstat(), no open() close() syscall, and no metadata from disk into memory, 
		 * so cannot distinguish DIR or FILE
		 */
		if (dir_depth >= pdi->dir_depth) {
			pdi->last_total++;					
			if (pdi->last_total % (1024 * 100) == 0) {
				snprintf(vd_orig, sizeof(vd_orig), "%ld", pdi->dirs_total);
				snprintf(vf_orig, sizeof(vf_orig), "%ld", pdi->files_total);
				snprintf(vl_orig, sizeof(vl_orig), "%ld", pdi->last_total);
				err_msg("thread:%3ld => P: %-40s dirs_total: %s  files_total: %s  last_total: %s  dir_depth[cur:%ld, set:%ld]   %s", pdi->tindex, dir,
						V(vd_orig, vd_new), V(vf_orig, vf_new), V(vl_orig, vl_new), dir_depth, pdi->dir_depth, datetime_now(date_buf));
			}			
			continue;
		}
		if (lstat(tmp, &stbuf) < 0) {
			err_msg("lstat(%s) error", tmp);
			continue;
		}
		if (S_ISDIR(stbuf.st_mode)) {
			pdi->dirs_total++;			
			if (pdi->all_delay == 0) {
				syscall_usleep(pdi->usecs);
			}
			if (dir_depth >= pdi->dir_depth) {
				continue;
			}
			recursive_dir(tmp, pdi, dir_depth + 1);
			continue;
		}
		if (S_ISREG(stbuf.st_mode)) {
			pdi->files_total++;
		}
	}
	closedir(dirp);
	return;
}

void *do_through_dir(void *arg)
{
	struct pthread_dir_info *pdi = (struct pthread_dir_info *) arg;
	long dir_depth = 1;

	recursive_dir(pdi->initial_dir, pdi, dir_depth);

	return 0;
}

void USAGE(char *prog)
{
	err_quit("Usage: %s [-D] [-n pthread_num] [-i interval(us)] [-I top_interval(s)] -d dir_depth", prog);
}

void print_original_cmdline(int argc, char **argv)
{
	int i;

	printf("\n**********   ORIGINAL COMMAND LINE:   ");
	for (i = 0; i < argc; i++) {
		printf("%s ", argv[i]);
	}
	printf("   **********\n");
	return;
}

void dump_pdi_info(struct pthread_dir_info *pdi)
{
	char vd_orig[64] = { 0 }, vd_new[64] = { 0 }, vf_orig[64] = { 0 }, vf_new[64] = { 0 }, vl_orig[64] = { 0 }, vl_new[64] = { 0 };
	if (pdi == NULL) {
		return;
	}
	printf("***************************************************\n");
	printf("pdi->tsum: %ld\n", pdi->tsum);
	printf("pdi->tindex: %ld\n", pdi->tindex);
	printf("pdi->initial_dir: %s\n", pdi->initial_dir);
	snprintf(vd_orig, sizeof(vd_orig), "%ld", pdi->dirs_total);
	printf("pdi->dirs_total: %s\n", V(vd_orig, vd_new));
	snprintf(vf_orig, sizeof(vf_orig), "%ld", pdi->files_total);
	printf("pdi->files_total: %s\n", V(vf_orig, vf_new));
	snprintf(vl_orig, sizeof(vl_orig), "%ld", pdi->last_total);
	printf("pdi->last_total: %s\n", V(vl_orig, vl_new));
	printf("pdi->dir_depth: %ld\n", pdi->dir_depth);
	printf("pdi->all_delay: %d\n", pdi->all_delay);
	printf("pdi->usecs: %ld\n", pdi->usecs);
	printf("***************************************************\n");
}

int main(int argc, char *argv[])
{
	struct pthread_dir_info pdi_array[MAX_PTHREAD_NUM] = { {0}, }, *pdi;
	pthread_t add_tid[MAX_PTHREAD_NUM] = { 0 };
	long pthread_num, dir_depth, interval, top_interval;
	int i, opt, work_times, cost_time, all_delay = 0;
	char tmpdir[PATH_MAX + 1] = { 0 }, date_buf[32] = { 0 };
	struct timeval tv_last = { 0 }, tv_now = { 0 };

	pthread_num = DEFAULT_PTHREAD_NUM;
	dir_depth = 0;
	interval = DEFAULT_INTERVAL_USECS;
	top_interval = DEFAULT_TOPLEVEL_INTERVAL_SECS;
	while ((opt = getopt(argc, argv, "n:Dd:hi:I:")) != -1) {
		switch (opt) {
			case 'n':
				pthread_num = strtoul(optarg, NULL, 10);
				break;
			case 'd':
				dir_depth = strtoul(optarg, NULL, 10);
				break;
			case 'D':
				all_delay = 1;
				break;
			case 'i':
				interval = strtoul(optarg, NULL, 10);
				break;
			case 'I':
				top_interval = strtoul(optarg, NULL, 10);
				break;
			case 'h':
			default:
				USAGE(argv[0]);
				break;
		}
	}
	//err_quit("argc[%d], %s, optind[%d]", argc, argv[optind], optind);
	if (argc != (optind)) {
		USAGE(argv[0]);
	}
	if (pthread_num > MAX_PTHREAD_NUM) {
		pthread_num = MAX_PTHREAD_NUM;
	}
	if (dir_depth > MAX_DIR_DEPTH) {
		dir_depth = MAX_DIR_DEPTH;
	}
	if (dir_depth == 0) {
		USAGE(argv[0]);
	}
	print_original_cmdline(argc, argv);
	err_msg("%s: pthread_num: [%ld], interval:[%ld], top_interval:[%ld], dir_depth:[%ld], all_delay: %d", argv[0], pthread_num, interval, top_interval, dir_depth, all_delay);

	if (gettimeofday(&tv_now, NULL) < 0) {
		err_sys("gettimeofday() error");
	}
	work_times = 0;
	while (1) {
		for (i = 0, pdi = pdi_array; i < pthread_num; i++, pdi++) {
			pdi->tsum = pthread_num;
			pdi->tindex = i;
			pdi->files_total = 0;
			pdi->dirs_total = 0;
			pdi->last_total = 0;
			pdi->dir_depth = dir_depth;
			pdi->all_delay = all_delay;
			pdi->usecs = interval;
			snprintf(tmpdir, sizeof(tmpdir), "%s%d", OSS_DATA_DIR_PREFIX, i + 1);
			pdi->initial_dir = strdup(tmpdir);

			if (pthread_create(&add_tid[i], NULL, do_through_dir, pdi) != 0) {
				perr_exit(errno, "pthread_create() error");
			}
		}

		for (i = 0, pdi = pdi_array; i < pthread_num; i++, pdi++) {
			pthread_join(add_tid[i], NULL);
			dump_pdi_info(pdi);
			if (pdi->initial_dir) {
				free(pdi->initial_dir);
			}
		}

		tv_last = tv_now;
		if (gettimeofday(&tv_now, NULL) < 0) {
			err_sys("gettimeofday() error");
		}
		cost_time = tv_now.tv_sec - tv_last.tv_sec;
		err_msg("\nfinished the '%d' work at [%s], cost: %d seconds, will go on after %ld seconds ...\n", ++work_times, datetime_now(date_buf), cost_time, top_interval);
		syscall_sleep(top_interval);
		tv_now.tv_sec += top_interval;
	}
	err_msg("finished all work!");

	return 0;
}
