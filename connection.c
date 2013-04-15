#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "dalloc.h"
#include "log.h"
#include "buff.h"
#include "connection.h"
#include "server.h"


static connection_pool_t *create_connection_pool(void);
static connection_t *create_connection(connection_pool_t *connection_pool, int fd);		
static void destroy_connection(connection_pool_t *connection_pool, connection_t *connection);
static void destroy_connection_pool(connection_pool_t *connection_pool);
static int check_max_connection(server_t *server);

static int inited = 0;
static connection_impl_t connection_impl;

//对外接口
connection_impl_t *connection_getinstance(void)
{
	connection_impl_t *ret = &connection_impl;	
	if(0 == inited)
	{
		ret->create_connection_pool = create_connection_pool;
		ret->create_connection = create_connection;
		ret->destroy_connection = destroy_connection;
		ret->destroy_connection_pool = destroy_connection_pool;
		ret->check_max_connection = check_max_connection;
		inited = 1;
	}
	return ret;
}

/**
 * @desc 创建连接池
 * @param void
 * @return connection_impl_t *
 */
connection_pool_t *create_connection_pool(void)
{
	connection_pool_t *connection_pool = NULL;
	recover_impl_t *recover_impl = recover_getinstance();
	if(NULL == (connection_pool = recover_impl->create_recover() ))
	{
		log_error("create connection_pool fail");	
		return NULL;
	}	
	return connection_pool;
}

/**
 * @desc 为新的连接分配内存
 * @param connection_pool_t *connection_pool 连接池
 * @param int fd 连接fd
 * @return connection_t *
 */
connection_t *create_connection(connection_pool_t *connection_pool, int fd)
{
	assert(fd > 0);	
	recover_impl_t *recover_impl = recover_getinstance();
	connection_t *connection = NULL;
	if(NULL == (connection = recover_impl->lend(connection_pool)))
	{
		DMALLOC(connection, connection_t *, sizeof(connection_t));	
		if(NULL == connection)
		{
			log_error("create_connection fail");
			return NULL;
		}
		if(NULL == (connection->buff = create_buff(CONNECTION_BUFF_DEFAULT_SIZE)))
		{
			log_error("create connection->buff fail");
			DFREE(connection);		
			return NULL;
		}
		//增加计算器
		recover_impl->incr_recover(connection_pool);
	}
	connection->fd = fd;	
	reset_buff(connection->buff);
	connection->read_len = 0;
	connection->have_read = 0;
	connection->have_write = 0;
	connection->reading = 0;
	connection->writing = 0;
	connection->begin_read_time = 0.0;
	connection->begin_write_time = 0.0;

	return connection;
}

/**
 * @desc 释放连接
 * @param connection_pool_t *connection_pool 连接池句柄
 * @param connection_t *connection 连接
 * @return void
 */
void destroy_connection(connection_pool_t *connection_pool, connection_t *connection)
{
	assert(connection);	
	recover_impl_t *recover_impl = recover_getinstance();
	recover_impl->giveback(connection_pool, connection);
}

/**
 * @desc 销毁内存池
 * @param connection_pool_t *connection_pool 内存池句柄
 * @return void
 */
void destroy_connection_pool(connection_pool_t *connection_pool)
{
	recover_impl_t *recover_impl = recover_getinstance();
	//TODO 处理池里面的connection
	recover_impl->destroy_recover(connection_pool);	
}

/**
 * @desc 判断程序释放能接收一个新的请求(由于无上锁，该函数非线程安全的)
 * @param server_t *server 服务器句柄 
 * @return int 1=还可以接,0=已经到达上限
 */
int check_max_connection(server_t *server)
{
	if(server->max_connection <= 0) return 1;	
	int curr_num = 0, i = 0;
	thread_t *thread = NULL;
	for(; i<server->thread_pool->thread_num; ++i)
	{
		thread = server->thread_pool->thread + i;
		curr_num += thread->connection_pool->total - thread->connection_pool->curr_num;	
	}
	if(curr_num >= server->max_connection) return 0;
	else return 1;
}
