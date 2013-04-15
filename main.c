#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <signal.h>

#include "log.h"
#include "server.h"

int handle_in_event(buff_t *buff); //处理input
int handle_out_event(buff_t *buff); //处理output
void server_end(int sig);
LOG_INIT

//日志记录级别
//uint32_t g_log_level = LOG_LEVEL_DEBUG;
uint32_t g_log_level = LOG_LEVEL_ERROR;

//日志记录文件夹
char g_log_dir[256] = "./logs/";
//日志记录方式
uint32_t g_log_show = LOG_SHOW_FILENAME;
server_t *g_server = NULL;

int main()
{
	signal(SIGINT, server_end);
	signal(SIGTERM, server_end);
	LOG_OPEN();

	server_config_t conf;
	memset(&conf, 0, sizeof(server_config_t));
	conf.port = 11111; //监听的端口
	conf.thread_num = 5; //线程数,少于1，将和cpu数量相等
	conf.read_timeout = 0.0; //读超时设置
	conf.write_timeout = 0.0; //写超时设置
	conf.handle_in_event = handle_in_event; //处理读逻辑
	conf.handle_out_event = handle_out_event; //处理写逻辑
	conf.max_connection = 0; //0=不限制

	server_impl_t *server_impl = server_getinstance();

	g_server = server_impl->create_server(&conf);
	if(0 != server_impl->run_server(g_server))
	{
		log_error("run server fail");
		return EXIT_FAILURE;		
	}
	server_impl->destroy_server(g_server);
	LOG_CLOSE();
	return EXIT_SUCCESS;	
}

/**
 * @desc 服务器接收本次请求的所有数据后，将会调用该函数
	typedef struct buff_s buff_t;
	buff_t是2进制安全的
	struct buff_s
	{
		void *data;
		int max_length;//当前data申请内存的数量
		int length;//当前长度
	};
	
 * @param buff_t *buff 本次请求的数据
 * @return int 0=保持当前连接，非0断开当前连接
 */
int handle_in_event(buff_t *buff)
{
	/*
	//使用例子
	unsigned char magic = 0xc9; 
	int package_header_len = sizeof(unsigned char) + sizeof(int);
	char username[20];
	char *ret_str = "OK";
	int ret_len = 0, userid = 0, age = 0, offset = package_header_len;
	//获取数据
	memcpy(&userid, buff->data + offset, sizeof(int));
	offset += sizeof(int);
	memcpy(username, buff->data + offset, 20 * sizeof(char));
	offset += 20 * sizeof(char);
	memcpy(&age, buff->data + offset, sizeof(int));

	//将buff清空(只是将buff->length设置为0)
	reset_buff(buff);

	ret_len = strlen(ret_str) + sizeof(unsigned char) + sizeof(int);
	//填充内容(input和output是公用同一个buff的)
	append_buff(buff, &magic, sizeof(unsigned char));
	append_buff(buff, &ret_len, sizeof(int));
	append_buff(buff, ret_str, strlen(ret_str));
	*/
	
	return 0;
}

/**
 * @desc 服务器接收本次请求的所有数据后，将会调用该函数
 * @param buff_t *buff 将要发送的内容
 * @return int 0=保持当前连接，非0断开当前连接
 */
int handle_out_event(buff_t *buff)
{
	//0保持当(长)连接，非0表示断开当前(短)连接
	return 0;	
}

/**
 * @desc 关闭服务器
 * @param int sig 信号
 * @return void
 */
void server_end(int sig)
{
	log_debug("start to shutdown server");
	server_getinstance()->send_shutdown_signal(g_server);
}

