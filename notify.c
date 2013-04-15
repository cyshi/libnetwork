#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "notify.h"

static int send_notify(int fd);	
static int recv_notify(int fd);	

static int inited = 0;
static notify_impl_t notify_impl;

notify_impl_t *notify_getinstance(void)
{
	notify_impl_t *ret = &notify_impl;	
	if(0 == inited)
	{
		ret->send_notify = send_notify;	
		ret->recv_notify = recv_notify;	
		inited = 0;
	}
	return ret;
}

/**
 * @desc 消息通知
 * @param int fd 消息管道
 * @return int 0=成功，-1=失败
 */
int send_notify(int fd)
{
	int ret = 0, cnt = 0;
	while(1)
	{
		ret = write(fd, "", 1);
		if(0 == ret) return -1;
		else if(-1 == ret)
		{
			if(errno == EINTR || errno == EAGAIN)
			{
				if(cnt++>100) return -1;
				continue;	
			}
			else return -1;
		}
		else break;
	}
	return 0;
}

/**
 * @desc 接收通知
 * @param int fd 消息管道
 * @return int  0=成功，-1=失败
 */
int recv_notify(int fd)
{
	char tmp;
	int ret = 0, cnt = 0;
	while(1)
	{
		ret = read(fd, &tmp, sizeof(char));	
		if(0 == ret) return -1;
		else if(-1 == ret)
		{
			if(EINTR == errno || EAGAIN == errno) 
			{
				if(cnt++>100) return -1;
				continue;
			}
			else return -1;
		}
		else break;
	}
	return 0;	
}

