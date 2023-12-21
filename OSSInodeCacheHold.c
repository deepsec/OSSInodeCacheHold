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
	char tmp[PATH_MAX] = {0};

    //dump_pdi_info(pdi);
    //return;
			
	if (dir == NULL) return;
	if ((dirp = opendir(dir)) == NULL) {
		//err_msg("opendir(%s) error", dir);
		return;
	}
	while ((dp = readdir(dirp)) != NULL) {
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
			continue;
		}
		snprintf(tmp, sizeof(tmp), "%s/%s", dir, dp->d_name);
		if (lstat(tmp, &stbuf) < 0) {
			err_msg("lstat(%s) error", tmp);
			continue;
		}
		if (S_ISDIR(stbuf.st_mode)) {
            pdi->dirs_total++;
            if (pdi->dirs_total % (4096) ==  0) {
                err_msg("thread: %ld, %s, dirs_total: %ld, files_total: %ld, dir_depth:%ld,%ld", pdi->tindex, tmp, pdi->dirs_total, pdi->files_total, dir_depth, pdi->dir_depth);
            }
            if (dir_depth >= pdi->dir_depth) {
                //return;
                continue;
            }
            syscall_usleep(pdi->usecs);
           	recursive_dir(tmp, pdi, dir_depth+1);
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
	struct pthread_dir_info  *pdi = (struct pthread_dir_info *) arg;
    long dir_depth = 1;
	
	recursive_dir(pdi->initial_dir, pdi, dir_depth);
    
	return 0;
}

void USAGE(char *prog)
{
    err_quit("Usage: %s [-n pthread_num] [-i interval(us)] -d dir_depth", prog);
}

void dump_pdi_info(struct pthread_dir_info *pdi)
{
        if (pdi == NULL) return;
        printf("***************************************************\n");
        printf("pdi->tsum: %ld\n", pdi->tsum);
        printf("pdi->tindex: %ld\n", pdi->tindex);
        printf("pdi->initial_dir: %s\n", pdi->initial_dir);
        printf("pdi->dirs_total: %ld\n", pdi->dirs_total);
        printf("pdi->files_total: %ld\n", pdi->files_total);
        printf("pdi->dir_depth: %ld\n", pdi->dir_depth);
        printf("pdi->usecs: %ld\n", pdi->usecs);
        printf("***************************************************\n");
}

int main(int argc, char *argv[])
{
	struct pthread_dir_info  pdi_array[MAX_PTHREAD_NUM] = {{0},}, *pdi;
    pthread_t   add_tid[MAX_PTHREAD_NUM] = {0};
	long pthread_num, dir_depth, interval;
    int i, opt;
    char tmpdir[PATH_MAX] = {0};

	pthread_num = DEFAULT_PTHREAD_NUM;
    dir_depth = 0;
    interval = DEFAULT_INTERVAL_USECS;
	while ((opt = getopt(argc, argv, "n:d:hi:")) != -1) {
		switch (opt) {
		case 'n':
			pthread_num = strtoul(optarg, NULL, 10);
			break;
		case 'd':
			dir_depth = strtoul(optarg, NULL, 10);
			break;
        case 'i':
            interval = strtoul(optarg, NULL, 10);
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
	
    pdi = pdi_array;
	for (i = 0; i < pthread_num; i++, pdi++) {	
		pdi->tsum = pthread_num;	
		pdi->tindex = i;
		pdi->files_total = 0;
		pdi->dirs_total = 0;
        pdi->dir_depth = dir_depth;
        pdi->usecs = interval;
        snprintf(tmpdir, sizeof(tmpdir), "%s%d", OSS_DATA_DIR_PREFIX, i+1);
        pdi->initial_dir = strdup(tmpdir);
		
		if (pthread_create(&add_tid[i], NULL, do_through_dir, pdi) != 0) {
			perr_exit(errno, "pthread_create() error");
		}
    }

    sleep(3);	

    pdi = pdi_array;
	for (i = 0; i < pthread_num; i++, pdi++) {		
		pthread_join(add_tid[i], NULL);       
        dump_pdi_info(pdi);
         if (pdi->initial_dir) {
            free(pdi->initial_dir);
        }
	}
	
	err_msg ("finished all work!");
	
	return 0;
}
