#ifndef _THREAD_H
#define _THREAD_H

#include <pthread.h>
#include "recover.h"

#include "connection.h"

struct server_s;

typedef enum thread_status_e thread_status_t;
typedef struct thread_s thread_t;
typedef struct thread_impl_s thread_impl_t;
typedef struct thread_pool_s thread_pool_t;

enum thread_status_e
{
  THREAD_STATUS_RUN = 0, //子线程正常运行
  THREAD_STATUS_SHUTDOWN = 1,//子线程在发送完数据包后，主动关闭连接
  THREAD_STATUS_DESTROY = 2 //子线程退出
};

struct thread_s
{
  struct thread_socket_s *thread_socket; //子线程socket句柄
  int event_fd[2]; //消息管道
  pthread_t tid; //子线程id
  connection_pool_t *connection_pool; //连接池
  int status; //子线程状态
  struct server_s *server; //server句柄
};

//线程池
struct thread_pool_s
{
  int thread_num; //线程数量
  thread_t *thread; //线程
};

//对外接口
struct thread_impl_s
{
  thread_pool_t *(*create_thread_pool)(int thread_num, struct server_s *server);	
  int (*start_thread)(struct server_s *server);
  int (*destroy_thread_pool)(thread_pool_t *thread_pool);
};

thread_impl_t *thread_getinstance(void);
#endif
