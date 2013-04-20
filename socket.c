#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>

#include "socket.h"
#include "thread.h"
#include "dalloc.h"
#include "log.h"
#include "fyutils.h"
#include "server.h"
#include "notify.h"

static main_socket_t *create_main_socket(int port);	
static int dispatch_main_socket(server_t *server);
static void destroy_main_socket(main_socket_t *main_socket);
static thread_socket_t *create_thread_socket(void);	
static int dispatch_thread_socket(thread_t *thread);
static void destroy_thread_socket(thread_socket_t *thread_socket);
static int handle_accept_event(thread_t *thread);
static int handle_epollin_event(thread_t *thread, connection_t *connection);
static int handle_epollout_event(thread_t *thread, connection_t *connection);
static int handle_epollerr_event(thread_t *thread, connection_t *connection);
static int handle_close(thread_t *thread, connection_t *connection, int close_fd);
static int active_epollin_event(thread_t *thread, connection_t *connection);
static int active_epollout_event(thread_t *thread, connection_t *connection);
static int check_write_timeout(thread_t *thread, connection_t *connection);
static int check_read_timeout(thread_t *thread, connection_t *connection);

static int inited = 0;
static socket_impl_t socket_impl;

//对外接口
socket_impl_t *socket_getinstance(void)
{
  socket_impl_t *ret = &socket_impl;	
  if(0 == inited)
  {
    ret->create_main_socket = create_main_socket;
    ret->dispatch_main_socket = dispatch_main_socket;
    ret->destroy_main_socket = destroy_main_socket;
    ret->create_thread_socket = create_thread_socket;
    ret->dispatch_thread_socket = dispatch_thread_socket;
    ret->destroy_thread_socket = destroy_thread_socket;
    inited = 1;	
  }
  return ret;
}

/**
 * @desc 创建监听句柄
 * @param int port 端口
 * @return main_socket_t *
 */
main_socket_t *create_main_socket(int port)
{
  assert(port > 0 && port < 65535);	
  main_socket_t *main_socket = NULL;
  DMALLOC(main_socket, main_socket_t *, sizeof(main_socket_t));
  if(NULL == main_socket)
  {
    log_error("create main_socket fail");	
    return NULL;
  }
  main_socket->port = port;
  main_socket->epfd = -1;
  main_socket->listenfd = -1;
  return main_socket;
}

/**
 * @desc 创建监听，并通知子线程有新的连接,主线程不做accept,也不做read/write
 * @param server_t *server 服务器句柄
 * @return int 0=成功，-1=失败
 */
int dispatch_main_socket(server_t *server)
{
  assert(server);
  int  i = 0, nfds = -1, tid = 0;
  struct epoll_event ev, events[1024];
  struct sockaddr_in main_socket_addr;
  notify_impl_t *notify_impl = notify_getinstance();

  main_socket_t *main_socket = server->main_socket;
  //创建监听句柄
  if(-1 == (main_socket->listenfd = socket(AF_INET, SOCK_STREAM, 0))) 
  {
    log_error("create socket fail, errno = %d", errno);
    return -1;
  }
  //对listenfd设置属性	
  if(-1 == setnonblocking(main_socket->listenfd))
  {
    log_error("setnonblocking fail, errno = %d", errno);
    return -1;
  }
  //设置端口复用
  if(-1 == setreuseaddr(main_socket->listenfd))
  {
    log_error("setreuseaddr fail, errno = %d", errno);
    return -1;
  }
  //设置nodelay
  if(-1 == setnodelay(main_socket->listenfd))
  {
    log_error("setnodelay fail, errno = %d", errno);
    return -1;
  }
  //设置其他属性
  if(-1 == setotheropt(main_socket->listenfd))
  {
    log_error("setotheropt fail, errno = %d", errno);
    return -1;
  }

  memset(&main_socket_addr, 0, sizeof(struct sockaddr_in));
  main_socket_addr.sin_family = AF_INET;    
  main_socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);   
  main_socket_addr.sin_port = htons(main_socket->port);    
  //绑定端口
  if(-1 == bind(main_socket->listenfd, 
        (struct sockaddr *)&main_socket_addr, 
        sizeof(struct sockaddr_in)))
  {
    log_error("bind fail, errno = %d", errno);
    return -1; 
  }
  //监听,准备接收连接   
  if(-1 == listen(main_socket->listenfd, SOMAXCONN))
  {
    log_error("listen fail, errno = %d", errno);
    return -1; 
  }

  //创建主线程的epoll句柄
  if(-1 == (main_socket->epfd = epoll_create(1024))) 
  {
    log_error("epoll_create fail, errno = %d", errno);
    return -1;
  }

  //将监听句柄加到主epoll里面去
  memset(&ev, 0, sizeof(struct epoll_event));
  ev.data.fd = main_socket->listenfd; 
  ev.events = EPOLLIN|EPOLLET;
  if(0 != epoll_ctl(main_socket->epfd, EPOLL_CTL_ADD, main_socket->listenfd, &ev))
  {   
    log_error("add listenfd %d to main epfd %d fail", 
        main_socket->listenfd, main_socket->epfd);
    return -1; 
  }   

  //循环派发任务
  while(SERVER_STATUS_DESTROY != server->status)
  {
    if(-1 != (nfds = epoll_wait(main_socket->epfd, events, 1024, EPOLL_WAIT_TIMEOUT)))
    {
      //log_debug("main nfds = %d", nfds);
      for(i=0; i<nfds; ++i)
      {
        //监听句柄有新的事件(新的连接)
        if(events[i].data.fd == main_socket->listenfd)
        {
          //当前服务器正在关闭中，拒绝新的连接
          if(SERVER_STATUS_RUN != server->status) continue;
          tid = server->cursor++ % server->thread_pool->thread_num;	
          if(0 != notify_impl->send_notify(server->thread_pool->thread[tid].event_fd[1]))
          {
            log_error("send_notify fail");	
          }
        }	
        else if(events[i].events & EPOLLERR)
        {
          log_error("main thread have error event, events=%d, errno=%d", events[i].events, errno);	
        }
        else if(events[i].events & EPOLLIN)
        {
          log_debug("main thread have epollin event");
        }
        else if(events[i].events & EPOLLOUT) 
        {
          log_debug("main thread have epollout event");
        }
        else
        {
          log_error("main thread have unknow epoll event, events=%d", events[i].events);		
        }
      }
    }
    else
    {
      if(errno == EINTR) continue;
      log_error("epoll_wait fail, errno=%d", errno);	
      break;
    }
  }
  return 0;	
}

/**
 * @desc 销毁主监听句柄
 * @param main_socket_t *main_socket
 * @return void
 */
void destroy_main_socket(main_socket_t *main_socket)
{
  assert(main_socket);	
  if(-1 != main_socket->listenfd) close(main_socket->listenfd);
  if(-1 != main_socket->epfd) close(main_socket->epfd);
  DFREE(main_socket);
}

/**
 * @desc 创建子线程socket句柄
 * @param void
 * @return thread_socket_t *
 */
thread_socket_t *create_thread_socket(void)
{
  thread_socket_t *thread_socket = NULL;
  DMALLOC(thread_socket, thread_socket_t *, sizeof(thread_socket_t));
  if(NULL == thread_socket)
  {
    log_error("create thread_socket fail");	
    return NULL;
  }
  thread_socket->epfd = -1;
  return thread_socket;
}

/**
 * @desc 出现化线程socket，并处理accept/read/write
 * @param thread_t *thread 线程句柄
 * @return int 0=成功,-1=失败
 */
int dispatch_thread_socket(thread_t *thread)
{
  assert(thread);
  int i = 0, nfds = -1;
  struct epoll_event ev, events[1024]; 
  connection_t *connection = NULL;

  if((thread->thread_socket->epfd = epoll_create(1024)) < 0)
  {
    log_error("thread epoll_create fail");	
    return -1;
  }

  memset(&ev, 0, sizeof(struct epoll_event));	
  ev.data.fd = thread->event_fd[0]; 
  ev.events = EPOLLIN|EPOLLET;
  if(0 != epoll_ctl(thread->thread_socket->epfd, EPOLL_CTL_ADD, thread->event_fd[0], &ev))
  {
    log_error("epoll_ctl add pipe fd %d fail, errno = %d", thread->event_fd[0], errno);	
    return -1;
  }

  while(THREAD_STATUS_DESTROY != thread->status)
  {
    if(-1 != (nfds = epoll_wait(thread->thread_socket->epfd, events, 1024, EPOLL_WAIT_TIMEOUT)))
    {
      //log_debug("thread nfds = %d", nfds);
      for(i=0; i<nfds; ++i)
      {
        if(events[i].data.fd == thread->event_fd[0])
        {
          if(0 != handle_accept_event(thread))
          {
            log_error("accept fail");	
          }
        }
        else if(events[i].events & EPOLLIN)
        {
          if(THREAD_STATUS_RUN != thread->status) continue; //关闭中拒绝新的请求
          connection = (connection_t *)events[i].data.ptr;	
          if(events[i].events & (EPOLLERR|EPOLLHUP))
          {
            log_debug("have epollhup event!");
            handle_close(thread, connection, 0);	
            continue;
          }
          handle_epollin_event(thread, connection);
        }
        else if(events[i].events & EPOLLOUT)
        {
          connection = (connection_t *)events[i].data.ptr;	
          if(0 != handle_epollout_event(thread, connection))
          {
            log_error("handle_epollin_event fail");	
          }
        }
        else if(events[i].events & EPOLLERR)
        {
          connection = (connection_t *)events[i].data.ptr;	
          if(0 != handle_epollerr_event(thread, connection))
          {
            log_error("handle_epollout_event fail");		
          }
        }
        else
        {
          connection = (connection_t *)events[i].data.ptr;	
          log_error("thread have epoll unknow events = %d", events[i].events);		
        }
      }
    }	
    else
    {
      if(errno == EINTR) continue;
      log_error("epoll_wait error nfds = %d, errno = %d", nfds, errno);
      break;		
    }
  }
  return 0;
}

/**
 * @desc 销毁线程socket句柄
 * @param thread_socket_t *thread_socket 线程socket
 * @return void
 */
void destroy_thread_socket(thread_socket_t *thread_socket)
{
  assert(thread_socket);	
  close(thread_socket->epfd);
  DFREE(thread_socket);
}

/**
 * @desc 处理accept事件
 * @param thread_t *thread 线程句柄
 * @return 0=成功,-1=失败
 */
int handle_accept_event(thread_t *thread)
{
  int cfd = -1;
  struct epoll_event ev;
  connection_impl_t *connection_impl = connection_getinstance();
  notify_impl_t *notify_impl = notify_getinstance();
  connection_t *connection = NULL;

  //接收通知	
  if(0 != notify_impl->recv_notify(thread->event_fd[0]))
  {
    log_error("recv_notify fail");	
    return -1;
  }

  //接收fd
  if(1 > (cfd = accept(thread->server->main_socket->listenfd, 0, 0)))
  {
    if(errno == EPROTO || errno == EINTR || errno == ECONNABORTED) return 0;
    else
    {
      log_error("accept fail form listenfd = %d, errno = %d", thread->server->main_socket->listenfd, errno);	
      return -1;
    }
  }

  //判断最大连接数(存在一定的误判的)
  if(0 == connection_impl->check_max_connection(thread->server))
  {
    close(cfd);	
    return 0;
  }

  //设置非阻塞
  if(0 != setnonblocking(cfd))
  {
    log_error("setnonblocking fd %d fail", cfd);	
    return -1;
  }

  //分配连接	
  if(NULL == (connection = connection_impl->create_connection(thread->connection_pool, cfd)))
  {
    log_error("create_connection fail");		
    close(cfd);
    return -1;
  }

  //将新增的fd添加到子线程epoll
  memset(&ev, 0, sizeof(struct epoll_event));
  ev.data.ptr = connection;
  ev.events = EPOLLIN|EPOLLET;
  if(0 != epoll_ctl(thread->thread_socket->epfd, EPOLL_CTL_ADD, cfd, &ev))
  {
    log_error("epoll_ctl fd %d fail", cfd);	
    close(cfd);
    return -1;
  }
  return 0;	
}

/**
 * @desc 处理epollin事件
 * @param thread_t *thread 线程句柄
 * @param connection_t *connection 连接句柄
 * @return int 0=成功,-1=失败
 */
int handle_epollin_event(thread_t *thread, connection_t *connection)
{
  log_debug("into handle_epollin_event");
  int l = 0, ret = 0, biz_ret = 0;	

  if(1 == connection->reading) ret = read(connection->fd, connection->buff->data + connection->buff->length, connection->read_len - connection->buff->length);
  else ret = read(connection->fd, connection->buff->data + connection->buff->length, 5 - connection->buff->length);

  if(0 == ret)
  {
    //连接已关闭
    log_debug("client have close");
    handle_close(thread, connection, 1);	
    return -1;
  }
  else if(-1 == ret)
  {
    if(EINTR == errno || EAGAIN == errno) return 0;	
    else
    {
      log_error("read fail, errno = %d", errno);		
      handle_close(thread, connection, 1);
      return -1;
    }
  }
  else
  {
    log_debug("connection->buff->length=%d,ret=%d", connection->buff->length, ret);
    connection->buff->length += ret;			

    if(0 == connection->read_len) 
    {
      if(PACKAGE_HEADER_MAGIC != *((unsigned char *)connection->buff->data))
      {
        log_error("bad package header, not 0x%x", PACKAGE_HEADER_MAGIC);	
        handle_close(thread, connection, 1);
        return -1;
      }
      memcpy(&connection->read_len, connection->buff->data + sizeof(unsigned char), sizeof(int));	
      //设置为正在读的状态
      connection->reading = 1;		

      //判断connection->buff是否需要扩容
      l = connection->read_len - connection->buff->max_length;
      if(l > 0)
      {
        if(0 != expand_buff(connection->buff, l))
        {
          log_error("expand_buff fail");		
          handle_close(thread, connection, 1);
          return -1;
        }	
      }
    }

    if(connection->buff->length < connection->read_len)
    {
      //数据还没有读完，继续收
      //判断超时
      if(1 == check_read_timeout(thread, connection))
      {
        //超时	
        log_debug("read timeout");
        handle_close(thread, connection, 1);
        return -1;
      }
      log_debug("recv again");
      active_epollin_event(thread, connection);
      return 0;	
    }
    else if(connection->buff->length > connection->read_len)
    {
      log_error("recv more data than I want!");
      handle_close(thread, connection, 1);
      return -1;
    }
    else
    {
      biz_ret = thread->server->handle_in_event(connection->buff);	
      if(0 == biz_ret)
      {
        connection->have_write = 0;
        connection->writing = 0;
        connection->reading = 0;
        active_epollout_event(thread, connection);					
      }
      else
      {
        log_debug("biz_ret not eq 0");
        handle_close(thread, connection, 1);		
      }
    }
  }
  return 0;
}

/**
 * @desc 处理epollout事件
 * @param thread_t *thread 线程句柄
 * @param connection_t *connection 连接句柄
 * @return int 0=成功,-1=失败
 */
int handle_epollout_event(thread_t *thread, connection_t *connection)
{
  log_debug("into handle_epollout_event");
  int biz_ret = 0, ret = 0;
  if(NULL != thread->server->handle_out_event && 1 != connection->writing)
  {
    biz_ret = thread->server->handle_out_event(connection->buff);	
  }	

  ret = write(connection->fd, connection->buff->data + connection->have_write, connection->buff->length - connection->have_write);
  log_debug("have_write=%d,ret=%d", connection->have_write, ret);
  if(0 == ret)
  {
    log_debug("write ret = 0");
    handle_close(thread, connection, 1);	
  }
  else if(-1  == ret)
  {
    if(EINTR == errno || EAGAIN == errno)
    {
      connection->writing = 1;
      return 0;	
    }
    else
    {
      log_error("write fail, errno = %d", errno);		
      handle_close(thread, connection, 1);
      return -1;
    }
  }
  else
  {
    connection->have_write += ret;
    if(connection->have_write < connection->buff->length)
    {
      connection->writing = 1;
      //判断超时
      if(1 == check_write_timeout(thread, connection))
      {
        log_debug("write timeout");
        handle_close(thread, connection, 1);	
        return -1;
      }
      active_epollout_event(thread, connection);
      return 0;	
    }
    else if(connection->have_write > connection->buff->length)
    {
      log_error("sys error send more data!!");	
      handle_close(thread, connection, 1);
      return -1;
    }
    else
    {
      if(0 == biz_ret)
      {
        connection->writing = 0;
        connection->reading = 0;
        connection->have_write = 0;
        connection->read_len = 0;
        reset_buff(connection->buff);
        active_epollin_event(thread, connection);	
      }
      else
      {
        log_error("biz_ret = %d", biz_ret);
        handle_close(thread, connection, 1);	
      }
    }
  }
  return 0;
}

/**
 * @desc 处理epollerr事件
 * @param thread_t *thread 线程句柄
 * @param connection_t *connection 连接句柄
 * @return int 0=成功,-1=失败
 */
int handle_epollerr_event(thread_t *thread, connection_t *connection)
{
  log_debug("have epollerr event");	
  return 0;
}

/**
 * @desc 关闭连接
 * @param thread_t *thread 线程句柄
 * @param connection_t *connection 连接句柄
 * @param int close_fd 是否关闭连接句柄 0=不关闭，1=关闭
 * @return int 0=成功,-1=失败
 */
int handle_close(thread_t *thread, connection_t *connection, int close_fd)
{
  recover_impl_t *recover_impl = recover_getinstance();
  if(close_fd) close(connection->fd);	
  reset_buff(connection->buff);
  recover_impl->giveback(thread->connection_pool, connection);
  return 0;
}

/**
 * @desc 将一个连接设置成epollin状态
 * @param thread_t *thread 线程句柄
 * @param connection_t *connection 连接句柄
 * @return int 0=成功,-1=失败
 */
int active_epollin_event(thread_t *thread, connection_t *connection)
{
  log_debug("into active_epollin_event");
  struct epoll_event ev;	
  ev.data.ptr = connection;
  ev.events = EPOLLIN|EPOLLET;
  if(0 != epoll_ctl(thread->thread_socket->epfd, EPOLL_CTL_MOD, connection->fd, &ev))
  {
    log_error("active_epollin_event epoll_ctl fail");	
    return -1;
  }
  return 0;
}

/**
 * @desc 将一个连接设置成epollout状态
 * @param thread_t *thread 线程句柄
 * @param connection_t *connection 连接句柄
 * @return int 0=成功,-1=失败
 */
int active_epollout_event(thread_t *thread, connection_t *connection)
{
  log_debug("into active_epollout_event");
  struct epoll_event ev;	
  ev.data.ptr = connection;
  ev.events = EPOLLOUT|EPOLLET;
  if(0 != epoll_ctl(thread->thread_socket->epfd, EPOLL_CTL_MOD, connection->fd, &ev))
  {
    log_error("active_epollout_event epoll_ctl fail");	
    return -1;
  }
  return 0;
}

/**
 * @desc 判断write有无超时
 * @param thread_t *thread 线程句柄
 * @param connection_t *connection 连接句柄
 * @return int 0=无超时,1=超时
 */
int check_write_timeout(thread_t *thread, connection_t *connection)
{
  if(thread->server->write_timeout <= 0.0) return 0;	
  double now = fytime();
  if(0.0 == connection->begin_write_time) connection->begin_write_time = now;	
  else
  {
    if(now - connection->begin_write_time > thread->server->write_timeout) return 1;		
  }
  return 0;
}

/**
 * @desc 判断read有无超时
 * @param thread_t *thread 线程句柄
 * @param connection_t *connection 连接句柄
 * @return int 0=无超时,1=超时
 */
int check_read_timeout(thread_t *thread, connection_t *connection)
{
  if(thread->server->read_timeout <= 0.0) return 0;	
  double now = fytime();
  if(0.0 == connection->begin_read_time) connection->begin_read_time = now;	
  else
  {
    if(now - connection->begin_read_time > thread->server->read_timeout) return 1;		
  }
  return 0;
}

