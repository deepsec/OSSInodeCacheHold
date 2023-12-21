#ifndef __DS_ERR_H__
#define __DS_ERR_H__

void err_ret(const char *fmt, ...);
void err_sys(const char *fmt, ...);
void err_exit(int error, const char *fmt, ...);
void err_dump(const char *fmt, ...);
void err_msg(const char *fmt, ...);
void err_quit(const char *fmt, ...);

void perr_exit(int error, const char *fmt, ...);
void perr_msg(const char *fmt, ...);
void perr_quit(const char *fmt, ...);

#define ERR_RET(fmt, args...)		err_ret("*ERR* %s[%d]: " fmt, __FILE__, __LINE__, ## args)
#define ERR_SYS(fmt, args...)		err_sys("*ERR* %s[%d]: " fmt, __FILE__, __LINE__, ## args)
#define ERR_EXIT(error, fmt, args...)	err_exit(error, "*ERR* %s[%d]: " fmt, __FILE__, __LINE__, ## args)
#define ERR_DUMP(fmt, args...)		err_dump("*ERR* %s[%d]: " fmt, __FILE__, __LINE__, ## args)
#define ERR_MSG(fmt, args...)		err_msg("*ERR* %s[%d]: " fmt, __FILE__, __LINE__, ## args)
#define ERR_QUIT(fmt, args...)		err_quit("*ERR* %s[%d]: " fmt, __FILE__, __LINE__, ## args)

#define PERR_EXIT(error, fmt, args...)	perr_exit(error, "*PERR* %s[%d]: " fmt, __FILE__, __LINE__, ## args)
#define PERR_MSG(fmt, args...)		perr_msg("*PERR* %s[%d]: " fmt, __FILE__, __LINE__, ## args)
#define PERR_QUIT(fmt, args...)		perr_quit("*PERR* %s[%d]: " fmt, __FILE__, __LINE__, ## args)

#ifdef __DS_DBG__
#define DS_DBG(fmt, args...)		err_msg("*DS_DBG* %s[%d] -> " fmt, __FILE__, __LINE__, ## args)
#define DS_PDBG(fmt, args...)		perr_msg("*DS_PDBG* %s[%d] -> " fmt, __FILE__, __LINE__, ## args)
#else
#define DS_DBG(fmt, args...)
#define DS_PDBG(fmt, args...)
#endif

#endif
