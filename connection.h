#ifndef _CONNECTION_H
#define _CONNECTION_H

#include "buff.h"
#include "recover.h"

struct server_s;

#define CONNECTION_BUFF_DEFAULT_SIZE 1024

typedef struct connection_s connection_t; 
typedef struct connection_impl_s connection_impl_t; 
typedef recover_t connection_pool_t;

struct connection_s
{
	int fd; //当前连接的句柄
	buff_t *buff; //连接的时候创建，断开连接的时候释放	
	int read_len;
	int have_read; //已经发送多少个字节
	int have_write; //已经发送多少个字节
	int reading; //当前是否正确发送数据
	int writing; //当前是否正确发送数据
	double begin_read_time;//开始读的时间(分包读，大于IO_BUFFER_SIZE)
	double begin_write_time;//开始写的时间(分包写)
};

//对外接口
struct connection_impl_s
{
	connection_pool_t *(*create_connection_pool)(void);
	connection_t *(*create_connection)(connection_pool_t *connection_pool, int fd);		
	void (*destroy_connection)(connection_pool_t *connection_pool, connection_t *connection);
	void (*destroy_connection_pool)(connection_pool_t *connection_pool);
	int (*check_max_connection)(struct server_s *server);
};

connection_impl_t *connection_getinstance(void);

#endif
