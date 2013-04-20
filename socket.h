#ifndef _SOCKET_H
#define _SOCKET_H

struct server_s;
struct thread_s;

#define EPOLL_EVENT_DEFAULT_NUM 1024;
#define EPOLL_WAIT_TIMEOUT 2000
#define PACKAGE_HEADER_MAGIC 0xc9

typedef struct socket_impl_s socket_impl_t;
typedef struct main_socket_s main_socket_t;
typedef struct thread_socket_s thread_socket_t;

struct main_socket_s
{
  int port; //监听的端口
  int epfd; //epoll句柄
  int listenfd; //监听句柄
};

struct thread_socket_s
{
  int epfd; //子线程epoll句柄
};

//对外接口
struct socket_impl_s
{
  main_socket_t *(*create_main_socket)(int port);	
  int (*dispatch_main_socket)(struct server_s *server);
  void (*destroy_main_socket)(main_socket_t *main_socket);
  thread_socket_t *(*create_thread_socket)(void);	
  int (*dispatch_thread_socket)(struct thread_s *thread);
  void (*destroy_thread_socket)(thread_socket_t *thread_socket);
};

socket_impl_t *socket_getinstance(void);
#endif
