#ifndef _SERVER_H
#define _SERVER_H

#include "thread.h"
#include "socket.h"
#include "buff.h"

typedef struct server_s server_t; 
typedef struct server_impl_s server_impl_t; 
typedef struct server_config_s server_config_t;
typedef enum server_status_e server_status_t;

enum server_status_e
{
  SERVER_STATUS_RUN = 0, //系统正常运行
  SERVER_STATUS_SHUTDOWN = 1,//系统停止接收新的连接	
  SERVER_STATUS_DESTROY = 2 //系统关闭，释放内存
};

struct server_config_s
{
  int port;
  int thread_num;
  int (*handle_in_event)(buff_t *buff); //业务处理函数
  int (*handle_out_event)(buff_t *buff); //业务处理函数
  double read_timeout; //读数据超时
  double write_timeout; //写数据超时
  int max_connection; //最大连接数
};

struct server_s
{
  struct main_socket_s *main_socket;
  unsigned int cursor; //连接游标,用于循环派发任务
  struct thread_pool_s *thread_pool;
  int status; //服务器状态
  int (*handle_in_event)(buff_t *buff); //业务处理函数
  int (*handle_out_event)(buff_t *buff); //业务处理函数
  double read_timeout; //读数据超时
  double write_timeout; //写数据超时
  int max_connection; //最大连接数
};

//系统对外接口
struct server_impl_s
{
  server_t *(*create_server)(server_config_t *conf);
  int (*run_server)(server_t *server);
  int (*destroy_server)(server_t *server);
  int (*send_shutdown_signal)(server_t *server);
};

//创建server实例
struct server_impl_s *server_getinstance(void);

#endif
