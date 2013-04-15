#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

#include "thread.h"
#include "log.h"
#include "fyutils.h"
#include "dalloc.h"
#include "connection.h"
#include "server.h"

static thread_pool_t *create_thread_pool(int thread_num, struct server_s *server);	
static int start_thread(struct server_s *server);
static int destroy_thread_pool(thread_pool_t *thread_pool);
static void *thread_func(void *user_data);

static int inited = 0;
static thread_impl_t thread_impl; 

//对外接口
thread_impl_t *thread_getinstance(void)
{
	thread_impl_t *ret = &thread_impl;	
	if(0 == inited)
	{
		ret->create_thread_pool = create_thread_pool;	
		ret->start_thread = start_thread;	
		ret->destroy_thread_pool = destroy_thread_pool;	
		inited = 1;
	}
	return ret;
}

/**
 * @desc 创建线程池
 * @param int thread_num 线程数量
 * @param server_t *server 服务器句柄 
 * @return thread_pool_t *
 */
thread_pool_t *create_thread_pool(int thread_num, server_t *server)
{
	int i = 0;
	thread_pool_t *thread_pool = NULL;	
	DMALLOC(thread_pool, thread_pool_t *, sizeof(thread_pool_t));
	if(NULL == thread_pool)
	{
		log_error("create thread_pool fail");	
		return NULL;
	}
	thread_pool->thread_num = thread_num;
	DCALLOC(thread_pool->thread, thread_t *, thread_num, sizeof(thread_t));
	if(NULL == thread_pool->thread)
	{
		DFREE(thread_pool);	
		return NULL;
	}
	for(; i<thread_num; ++i)
	{
		thread_pool->thread[i].event_fd[0] = -1;	
		thread_pool->thread[i].event_fd[1] = -1;	
		thread_pool->thread[i].tid = -1;	
		thread_pool->thread[i].server = server;	
	}
	return thread_pool;
}

/**
 * @desc 启动线程
 * @param server_t *server 服务器句柄 
 * @return int 0=成功，-1=失败
 */
int start_thread(server_t *server)
{
	assert(server);
	int i = 0;
	thread_pool_t *thread_pool = server->thread_pool;
	thread_t *thread = NULL;
	for(; i<thread_pool->thread_num; ++i)
	{
		thread = thread_pool->thread + i;	
		if(0 != pthread_create(&thread->tid, NULL, thread_func, thread))
		{
			log_error("pthread_create fail");	
			return -1;
		}
	}
	return 0;	
}

/**
 * @desc 销毁线程池
 * @param thread_pool_t *thread_pool 线程池句柄
 * @return int 
 */
int destroy_thread_pool(thread_pool_t *thread_pool)
{
	int i = 0;
	thread_t *thread = NULL;
	recover_impl_t *recover_impl = recover_getinstance();
	socket_impl_t *socket_impl = socket_getinstance();
	for(; i<thread_pool->thread_num; ++i)
	{
		thread = thread_pool->thread + i;
		if(0 != pthread_join(thread->tid, NULL)) log_error("pthread_join thread %d fail", i);	
		close(thread->event_fd[0]);	
		close(thread->event_fd[1]);	
		recover_impl->destroy_recover(thread->connection_pool);
		socket_impl->destroy_thread_socket(thread->thread_socket);
	}
	DFREE(thread_pool->thread);
	DFREE(thread_pool);
	return 0;	
}

/**
 * @desc 子线程处理函数
 * @param void *user_data 参数
 * @return void *
 */
void *thread_func(void *user_data)
{
	thread_t *thread = (thread_t *)user_data;
	socket_impl_t *socket_impl = socket_getinstance();
	connection_impl_t *connection_impl = connection_getinstance();

	//创建管道
	if(0 != pipe(thread->event_fd))
	{
		log_error("pipe fail");	
		return NULL;
	}	
	//设置非阻塞
	if(0 != setnonblocking(thread->event_fd[0]))
	{
		log_error("setnonblocking thread event_fd 0 fail");	
		return NULL;
	}	
	//设置非阻塞
	if(0 != setnonblocking(thread->event_fd[1]))
	{
		log_error("setnonblocking thread event_fd 1 fail");	
		return NULL;
	}	
	//创建连接池
	if(NULL == (thread->connection_pool = connection_impl->create_connection_pool()))
	{
		log_error("create connection_pool fail");	
		return NULL;
	}
	//创建线程socket	
	if(NULL == (thread->thread_socket = socket_impl->create_thread_socket()))
	{
		log_error("create thread_socket fail");	
		return NULL;
	}	

	//处理accept/read/write事件
	if(0 != socket_impl->dispatch_thread_socket(thread))
	{
		log_error("dispatch_thread_socket fail");	
		return NULL;	
	}

	return NULL;	
}

