#ifndef _NOTIFY_H
#define _NOTIFY_H

typedef struct notify_impl_s notify_impl_t;

struct notify_impl_s
{
	int (*send_notify)(int fd);	
	int (*recv_notify)(int fd);	
};

notify_impl_t *notify_getinstance(void);

#endif
