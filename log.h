#ifndef _LOG_H
#define _LOG_H

#include <time.h>
#include <pthread.h>
#include <stdint.h>

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_BIZ 1
#define LOG_LEVEL_ERROR 2
#define LOG_LEVEL_WARNING 3
#define LOG_LEVEL_NOTICE 4
#define LOG_LEVEL_DEBUG 5

#define LOG_SHOW_BOTH 0
#define LOG_SHOW_STDERR 1
#define LOG_SHOW_FILENAME 2

//#define FY_NO_LOG

#define LOG_INIT \
FILE *g_log_debug_fp = NULL;\
FILE *g_log_notice_fp = NULL;\
FILE *g_log_warning_fp = NULL;\
FILE *g_log_error_fp = NULL;\
FILE *g_log_biz_fp = NULL;\
uint32_t g_log_biz_day = 0;\
uint32_t g_log_error_day = 0;\
uint32_t g_log_warning_day = 0;\
uint32_t g_log_notice_day = 0;\
uint32_t g_log_debug_day = 0;\
pthread_mutex_t g_log_debug_lock = PTHREAD_MUTEX_INITIALIZER;\
pthread_mutex_t g_log_notice_lock = PTHREAD_MUTEX_INITIALIZER;\
pthread_mutex_t g_log_warning_lock = PTHREAD_MUTEX_INITIALIZER;\
pthread_mutex_t g_log_error_lock = PTHREAD_MUTEX_INITIALIZER;\
pthread_mutex_t g_log_biz_lock = PTHREAD_MUTEX_INITIALIZER;

#ifndef FY_NO_LOG
#define LOG_OPEN() \
do{\
	time_t t = time(NULL);\
	struct tm * p = localtime(&t);\
	char time_str[9];\
	memset(time_str, 0, 9);\
	strftime(time_str, 9, "%Y%m%d", p);\
	g_log_debug_day = g_log_error_day = g_log_warning_day = g_log_notice_day = g_log_biz_day = (uint32_t)atoi(time_str);\
	if(g_log_level >= LOG_LEVEL_DEBUG)\
	{\
		char filename[512];\
		memset(filename, 0, 512);\
		sprintf(filename,"%s%s_%u.log", g_log_dir, "debug", g_log_debug_day);\
		g_log_debug_fp = fopen(filename, "a+");\
		if(NULL == g_log_debug_fp)\
		{\
			fprintf(stderr, "fopen debug log %s fail\n", filename);\
			exit(1);\
		}\
	}\
	if(g_log_level >= LOG_LEVEL_NOTICE)\
	{\
		char filename[512];\
		memset(filename, 0, 512);\
		sprintf(filename,"%s%s_%u.log", g_log_dir, "notice", g_log_notice_day);\
		g_log_notice_fp = fopen(filename, "a+");\
		if(NULL == g_log_notice_fp)\
		{\
			fprintf(stderr, "fopen notice log %s fail\n", filename);\
			exit(1);\
		}\
	}\
	if(g_log_level >= LOG_LEVEL_WARNING)\
	{\
		char filename[512];\
		memset(filename, 0, 512);\
		sprintf(filename,"%s%s_%u.log", g_log_dir, "warning", g_log_warning_day);\
		g_log_warning_fp = fopen(filename, "a+");\
		if(NULL == g_log_warning_fp)\
		{\
			fprintf(stderr, "fopen warning log %s fail\n", filename);\
			exit(1);\
		}\
	}\
	if(g_log_level >= LOG_LEVEL_ERROR)\
	{\
		char filename[512];\
		memset(filename, 0, 512);\
		sprintf(filename,"%s%s_%u.log", g_log_dir, "error", g_log_error_day);\
		g_log_error_fp = fopen(filename, "a+");\
		if(NULL == g_log_error_fp)\
		{\
			fprintf(stderr, "fopen error log %s fail\n", filename);\
			exit(1);\
		}\
	}\
	if(g_log_level >= LOG_LEVEL_BIZ)\
	{\
		char filename[512];\
		memset(filename, 0, 512);\
		sprintf(filename,"%s%s_%u.log", g_log_dir, "biz", g_log_biz_day);\
		g_log_biz_fp = fopen(filename, "a+");\
		if(NULL == g_log_biz_fp)\
		{\
			fprintf(stderr, "fopen biz log %s fail\n", filename);\
			exit(1);\
		}\
	}\
}while(0)


#define LOG_CLOSE() \
do{\
	if(g_log_level >= LOG_LEVEL_DEBUG) fclose(g_log_debug_fp);\
	if(g_log_level >= LOG_LEVEL_NOTICE) fclose(g_log_notice_fp);\
	if(g_log_level >= LOG_LEVEL_WARNING) fclose(g_log_warning_fp);\
	if(g_log_level >= LOG_LEVEL_ERROR) fclose(g_log_error_fp);\
	if(g_log_level >= LOG_LEVEL_BIZ) fclose(g_log_biz_fp);\
}while(0)

extern uint32_t g_log_level;
extern char g_log_dir[256];
extern FILE *g_log_debug_fp;
extern FILE *g_log_notice_fp;
extern FILE *g_log_warning_fp;
extern FILE *g_log_error_fp;
extern FILE *g_log_biz_fp;
extern uint32_t g_log_biz_day;
extern uint32_t g_log_error_day;
extern uint32_t g_log_warning_day;
extern uint32_t g_log_notice_day;
extern uint32_t g_log_debug_day;
extern pthread_mutex_t g_log_debug_lock;
extern pthread_mutex_t g_log_notice_lock;
extern pthread_mutex_t g_log_warning_lock;
extern pthread_mutex_t g_log_error_lock;
extern pthread_mutex_t g_log_biz_lock;
extern uint32_t g_log_show;

#define log_debug(fmt, args...) \
do{\
	if(g_log_level >= LOG_LEVEL_DEBUG)\
	{\
		pthread_mutex_lock(&g_log_debug_lock);\
		time_t t = time(NULL);\
		struct tm * p = localtime(&t);\
		char time_str[20];\
		memset(time_str, 0, 20);\
		char time_str_day[9];\
		memset(time_str_day, 0, 9);\
		strftime(time_str_day,9,"%Y%m%d",p);\
		strftime(time_str,20,"%Y-%m-%d %H:%M:%S",p);\
		uint32_t today = (uint32_t)atoi(time_str_day);\
		if(today != g_log_debug_day)\
		{\
			g_log_debug_day = today;\
			fclose(g_log_debug_fp);\
			char filename[512];\
			memset(filename, 0, 512);\
			sprintf(filename,"%s%s_%u.log", g_log_dir, "debug", g_log_debug_day);\
			g_log_error_fp = fopen(filename, "a+");\
			if(NULL == g_log_debug_fp)\
			{\
				fprintf(stderr, "fopen debug log %s fail\n", filename);\
				exit(1);\
			}\
		}\
		if(g_log_show != LOG_SHOW_FILENAME) fprintf(stderr, "%s %s:%d(%s):" fmt "\n", time_str, __FILE__, __LINE__, __FUNCTION__, ## args);\
		if(g_log_show != LOG_SHOW_STDERR) fprintf(g_log_debug_fp, "%s %s:%d(%s):" fmt "\n", time_str, __FILE__, __LINE__, __FUNCTION__, ## args);\
		fflush(g_log_debug_fp);\
		pthread_mutex_unlock(&g_log_debug_lock);\
	}\
}while(0)

#define log_notice(fmt, args...) \
do{\
	if(g_log_level >= LOG_LEVEL_NOTICE)\
	{\
		pthread_mutex_lock(&g_log_notice_lock);\
		time_t t = time(NULL);\
		struct tm * p = localtime(&t);\
		char time_str[20];\
		memset(time_str, 0, 20);\
		char time_str_day[9];\
		memset(time_str_day, 0, 9);\
		strftime(time_str_day,9,"%Y%m%d",p);\
		strftime(time_str,20,"%Y-%m-%d %H:%M:%S",p);\
		uint32_t today = atoi(time_str_day);\
		if(today != g_log_notice_day)\
		{\
			g_log_notice_day = today;\
			fclose(g_log_notice_fp);\
			char filename[512];\
			memset(filename, 0, 512);\
			sprintf(filename,"%s%s_%u.log", g_log_dir, "notice", g_log_notice_day);\
			g_log_error_fp = fopen(filename, "a+");\
			if(NULL == g_log_notice_fp)\
			{\
				fprintf(stderr, "fopen notice log %s fail\n", filename);\
				exit(1);\
			}\
		}\
		if(g_log_show != LOG_SHOW_FILENAME) fprintf(stderr, "%s %s:%d(%s):" fmt "\n", time_str, __FILE__, __LINE__, __FUNCTION__, ## args);\
		if(g_log_show != LOG_SHOW_STDERR) fprintf(g_log_notice_fp, "%s %s:%d(%s):" fmt "\n", time_str, __FILE__, __LINE__, __FUNCTION__, ## args);\
		fflush(g_log_notice_fp);\
		pthread_mutex_unlock(&g_log_notice_lock);\
	}\
}while(0)

#define log_warning(fmt, args...) \
do{\
	if(g_log_level >= LOG_LEVEL_WARNING)\
	{\
		pthread_mutex_lock(&g_log_warning_lock);\
		time_t t = time(NULL);\
		struct tm * p = localtime(&t);\
		char time_str[20];\
		memset(time_str, 0, 20);\
		char time_str_day[9];\
		memset(time_str_day, 0, 9);\
		strftime(time_str_day,9,"%Y%m%d",p);\
		strftime(time_str,20,"%Y-%m-%d %H:%M:%S",p);\
		uint32_t today = atoi(time_str_day);\
		if(today != g_log_warning_day)\
		{\
			g_log_warning_day = today;\
			fclose(g_log_warning_fp);\
			char filename[512];\
			memset(filename, 0, 512);\
			sprintf(filename,"%s%s_%u.log", g_log_dir, "warning", g_log_warning_day);\
			g_log_error_fp = fopen(filename, "a+");\
			if(NULL == g_log_warning_fp)\
			{\
				fprintf(stderr, "fopen warning log %s fail\n", filename);\
				exit(1);\
			}\
		}\
		if(g_log_show != LOG_SHOW_FILENAME) fprintf(stderr, "%s %s:%d(%s):" fmt "\n", time_str, __FILE__, __LINE__, __FUNCTION__, ## args);\
		if(g_log_show != LOG_SHOW_STDERR) fprintf(g_log_warning_fp, "%s %s:%d(%s):" fmt "\n", time_str, __FILE__, __LINE__, __FUNCTION__, ## args);\
		fflush(g_log_warning_fp);\
		pthread_mutex_unlock(&g_log_warning_lock);\
	}\
}while(0)

#define log_error(fmt, args...) \
do{\
	if(g_log_level >= LOG_LEVEL_ERROR)\
	{\
		pthread_mutex_lock(&g_log_error_lock);\
		time_t t = time(NULL);\
		struct tm * p = localtime(&t);\
		char time_str[20];\
		memset(time_str, 0, 20);\
		char time_str_day[9];\
		memset(time_str_day, 0, 9);\
		strftime(time_str_day,9,"%Y%m%d",p);\
		strftime(time_str,20,"%Y-%m-%d %H:%M:%S",p);\
		uint32_t today = atoi(time_str_day);\
		if(today != g_log_error_day)\
		{\
			g_log_error_day = today;\
			fclose(g_log_error_fp);\
			char filename[512];\
			memset(filename, 0, 512);\
			sprintf(filename,"%s%s_%u.log", g_log_dir, "error", g_log_error_day);\
			g_log_error_fp = fopen(filename, "a+");\
			if(NULL == g_log_error_fp)\
			{\
				fprintf(stderr, "fopen error log %s fail\n", filename);\
				exit(1);\
			}\
		}\
		if(g_log_show != LOG_SHOW_FILENAME) fprintf(stderr, "%s %s:%d(%s):" fmt "\n", time_str, __FILE__, __LINE__, __FUNCTION__, ## args);\
		if(g_log_show != LOG_SHOW_STDERR) fprintf(g_log_error_fp, "%s %s:%d(%s):" fmt "\n", time_str, __FILE__, __LINE__, __FUNCTION__, ## args);\
		fflush(g_log_error_fp);\
		pthread_mutex_unlock(&g_log_error_lock);\
	}\
}while(0)

#define log_biz(fmt, args...) \
do{\
	if(g_log_level >= LOG_LEVEL_BIZ)\
	{\
		pthread_mutex_lock(&g_log_biz_lock);\
		time_t t = time(NULL);\
		struct tm * p = localtime(&t);\
		char time_str[20];\
		memset(time_str, 0, 20);\
		char time_str_day[9];\
		memset(time_str_day, 0, 9);\
		strftime(time_str_day,9,"%Y%m%d",p);\
		strftime(time_str,20,"%Y-%m-%d %H:%M:%S",p);\
		uint32_t today = atoi(time_str_day);\
		if(today != g_log_biz_day)\
		{\
			g_log_biz_day = today;\
			fclose(g_log_biz_fp);\
			char filename[512];\
			memset(filename, 0, 512);\
			sprintf(filename,"%s%s_%u.log", g_log_dir, "biz", g_log_biz_day);\
			g_log_biz_fp = fopen(filename, "a+");\
			if(NULL == g_log_biz_fp)\
			{\
				fprintf(stderr, "fopen biz log %s fail\n", filename);\
				exit(1);\
			}\
		}\
		if(g_log_show != LOG_SHOW_FILENAME) fprintf(stderr, "%s %s:%d(%s):" fmt "\n", time_str, __FILE__, __LINE__, __FUNCTION__, ## args);\
		if(g_log_show != LOG_SHOW_STDERR) fprintf(g_log_biz_fp, "%s %s:%d(%s):" fmt "\n", time_str, __FILE__, __LINE__, __FUNCTION__, ## args);\
		fflush(g_log_biz_fp);\
		pthread_mutex_unlock(&g_log_biz_lock);\
	}\
}while(0)
#else

#define LOG_OPEN() do{}while(0) 
#define LOG_CLOSE() do{}while(0) 
#define log_biz(fmt, args...) do{printf("[biz]" fmt "\n", ## args);}while(0) 
#define log_error(fmt, args...) do{printf("[error]" fmt "\n", ## args);}while(0) 
#define log_warning(fmt, args...) do{printf("[warning]" fmt "\n", ## args);}while(0) 
#define log_notice(fmt, args...) do{printf("[notice]" fmt "\n", ## args);}while(0) 
#define log_debug(fmt, args...) do{printf("[debug]" fmt "\n", ## args);}while(0) 

#endif
#endif
