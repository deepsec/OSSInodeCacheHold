#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "ds_common.h"
#include "ds_err.h"

/* Read "n" bytes from a descriptor  */
ssize_t readn(int fd, void *ptr, size_t n)
{
	size_t nleft;
	ssize_t nread;

	nleft = n;
	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (nleft == n)
				return (-1);	/* error, return -1 */
			else
				break;	/* error, return amount read so far */
		}
		else if (nread == 0) {
			break;	/* EOF */
		}
		nleft -= nread;
		ptr += nread;
	}
	return (n - nleft);	/* return >= 0 */
}

/* Write "n" bytes to a descriptor  */
ssize_t writen(int fd, const void *ptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;

	nleft = n;
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) < 0) {
			if (nleft == n)
				return (-1);	/* error, return -1 */
			else
				break;	/* error, return amount written so far */
		}
		else if (nwritten == 0) {
			break;
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return (n - nleft);	/* return >= 0 */
}

/* translate  '123456' to '123,456' for easy read */
char *V(char *d, char *r)
{
	int	i, len, left, blocks;
	char *pd = d, *pr = r;

	if (d == NULL || r == NULL)	return NULL;
	len = strlen(d);
	blocks = len / 3;
	left = len % 3;
	if (left > 0) {
		strncpy(pr, pd, left);
		pr += left;
		pd += left;
		*pr++ = ',';
	}
	for (i = 0; i < blocks; i++) {
		strncpy(pr, pd, 3);
		pr += 3;
		pd += 3;
		*pr++ = ',';
	}
	/* replace the last ',' to '\0' */
	*(pr-1) = '\0';
	return r;
}

char *datetime_now(char *buf)
{
	struct timeval tv = {0};
	char *datetime = NULL;

	if (buf == NULL) {
		return NULL;
	}
	if (gettimeofday(&tv, NULL) < 0) {
		err_sys("gettimeofday() error");
	}	
	datetime = ctime_r(&tv.tv_sec, buf);
	datetime[strlen(datetime) - 1] = '\0';

	return datetime;
}