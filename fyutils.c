#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/epoll.h>
#include <time.h>
#include <sys/time.h>

#include "fyutils.h"


int setnonblocking(int fd)
{
    int opts;
    if(0 > (opts = fcntl(fd,F_GETFL)))
    {
        fprintf(stderr, "fcntl(sock,GETFL)\n");
        return -1;
    }
    opts = opts|O_NONBLOCK;
    if (fcntl(fd,F_SETFL,opts) < 0)
    {
        fprintf(stderr, "fcntl(sock,SETFL,opts)\n");
        return -1;
    }
	return 0;
}

int setreuseaddr(int fd)
{
    int opt = 1;
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(&opt))) return -1; 
    else return 0;
}

int setnodelay(int fd)
{
    int opt = 1; 
    if(-1 == setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(&opt))) return -1;
    else return 0;
}

int setotheropt(int fd)
{
	int opt = 1;
	struct linger ling;
	ling.l_onoff = 0;
	ling.l_linger = 0;
	if(-1 == setsockopt(fd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling))) return -1;
    if(-1 == setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt))) return -1;
	return 0;
}

uint64_t get_curr_time(void)
{
	struct timeval t;
	if(0 != gettimeofday(&t, NULL)) return 0;
	else return t.tv_sec * 1000000 + t.tv_usec; 
}

/**
 * @desc 判断是否是一个文件
 * @param const char *filename 文件名称
 * @return 0=不是一个文件，1=是一个文件
 */
int is_file(const char *filename)
{
	return (0 == access(filename, R_OK))?1:0;	
}

/**
 * @desc 判断是否是一个文件夹
 * @param const char *dirname 文件夹名称
 * @return 0=不是一个文件夹，1=是一个文件夹
 */
int is_dir(const char *dirname)
{
	assert(dirname);
	struct stat sbuf;
	if(-1 == stat(dirname, &sbuf)) return -1;
	if(!S_ISDIR(sbuf.st_mode)) return 0;
	else return 1;
}

double fytime(void)
{
	struct timeval tv;
	if(gettimeofday(&tv, NULL) == -1) return 0.0;
	return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

/**
 * @desc 获取当前系统的负荷
 * @param void
 * @return double
 */
double ttgetloadavg(void)
{
	double avgs[3];
	int anum = getloadavg(avgs, sizeof(avgs) / sizeof(*avgs));
	if(anum < 1) return 0.0;
	return anum == 1 ? avgs[0] : avgs[1];
}


