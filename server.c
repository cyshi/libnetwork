#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "server.h"
#include "dalloc.h"
#include "log.h"

static server_t *create_server(server_config_t *conf);
static int run_server(server_t *server);
static int destroy_server(server_t *server);
static int send_shutdown_signal(server_t *server);

static int inited = 0;
static server_impl_t server_impl;

//对外接口
server_impl_t *server_getinstance(void)
{
	server_impl_t *ret = &server_impl;	
	if(0 == inited)
	{
		ret->create_server = create_server;	
		ret->run_server = run_server;	
		ret->destroy_server = destroy_server;	
		ret->send_shutdown_signal = send_shutdown_signal;	
		inited = 1;
	}
	return ret;
}

/**
 * @desc 创建服务器句柄
 * @param server_config_t *conf 服务器配置
 * @return server_t *
 */
server_t *create_server(server_config_t *conf)
{
	assert(conf);
	if(conf->port < 1 || conf->port > 65535)
	{
		log_error("bad port %d", conf->port);		
		return NULL;
	}
	if(NULL == conf->handle_in_event)
	{
		log_error("conf handle_in_event is NULL");	
		return NULL;
	}
	if(conf->max_connection < 0) conf->max_connection = 0;
	if(conf->read_timeout < 0.0) conf->read_timeout = 0.0;
	if(conf->write_timeout < 0.0) conf->write_timeout = 0.0;
		
	//当线程数小于1的时候，就获得当前cpu的数量，并将线程设置为cpu的数量
	if(conf->thread_num <= 0) conf->thread_num = sysconf(_SC_NPROCESSORS_ONLN);

	server_t *server = NULL;	
	socket_impl_t *socket_impl = socket_getinstance();
	thread_impl_t *thread_impl = thread_getinstance();

	DMALLOC(server, server_t *, sizeof(server_t));
	if(NULL == server)
	{
		log_error("create server fail");		
		return NULL;
	}
	server->status = SERVER_STATUS_RUN;
	server->cursor = 0; //任务派发游标
	server->handle_in_event = conf->handle_in_event;
	server->handle_out_event = conf->handle_out_event;
	server->read_timeout = conf->read_timeout;
	server->write_timeout = conf->write_timeout;
	server->max_connection = conf->max_connection;

	//创建主监听socket
	if(NULL == (server->main_socket = socket_impl->create_main_socket(conf->port)))
	{
		log_error("create main_socket fail");	
		DFREE(server);
		return NULL;
	}	

	//创建线程socket
	if(NULL == (server->thread_pool = thread_impl->create_thread_pool(conf->thread_num, server)))
	{
		log_error("create thread_pool fail");	
		socket_impl->destroy_main_socket(server->main_socket);
		DFREE(server);
		return NULL;
	}
	return server;	
}

/**
 * @desc 运行服务器
 * @param server_t *server 服务器句柄
 * @return 0=成功,-1=失败
 */
int run_server(server_t *server)
{
	assert(server);
	socket_impl_t *socket_impl = socket_getinstance();
	thread_impl_t *thread_impl = thread_getinstance();

	//要启动子线程才能开始监听的
	if(0 != thread_impl->start_thread(server))
	{
		log_error("start thread fail");		
		return -1;
	}

	//启动监听
	if(0 != socket_impl->dispatch_main_socket(server))
	{
		log_error("dispatch_main_socket fail");		
		return -1;
	}
	return 0;	
}

/**
 * @desc 销毁服务器
 * @param server_t *server 服务器句柄
 * @return int 0=成功,-1=失败
 */
int destroy_server(server_t *server)
{
	assert(server);
	socket_impl_t *socket_impl = socket_getinstance();
	thread_impl_t *thread_impl = thread_getinstance();

	socket_impl->destroy_main_socket(server->main_socket);

	thread_impl->destroy_thread_pool(server->thread_pool);

	DFREE(server);	
	return 0;
}

/**
 * @desc 发送关闭信号
 * @param server_t *server 服务器句柄
 * @return int
 */
int send_shutdown_signal(server_t *server)
{
	int i = 0;
	server->status = SERVER_STATUS_SHUTDOWN; //拒绝新的连接			
	for(i=0; i<server->thread_pool->thread_num; ++i) server->thread_pool->thread[i].status = THREAD_STATUS_SHUTDOWN; //拒绝EPOLLIN,留点时间处理EPOLLOUT	
	sleep(2);
	for(i=0; i<server->thread_pool->thread_num; ++i) server->thread_pool->thread[i].status = THREAD_STATUS_DESTROY; //关闭	
	server->status = SERVER_STATUS_DESTROY; //关闭			
	return 0;	
}
