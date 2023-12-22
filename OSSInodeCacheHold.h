#ifndef __OSSINODECACHEHODL_H__
#define __OSSINODECACHEHODL_H__

struct pthread_dir_info {
	long tsum;
	long tindex;
	char *initial_dir;
	long dir_depth;
	long dirs_total;
	long files_total;
	long usecs;
};

#define OSS_DATA_DIR_PREFIX				"/srv/node/d"
#define DEFAULT_DIR_DEPTH	 			3
#define MAX_DIR_DEPTH					6
#define MAX_PTHREAD_NUM					36
#define DEFAULT_PTHREAD_NUM				12
#define DEFAULT_INTERVAL_USECS			500
#define DEFAULT_TOPLEVEL_INTERVAL_SECS	1200


void dump_pdi_info(struct pthread_dir_info *pdi);
#endif
