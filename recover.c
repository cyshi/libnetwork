#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "dalloc.h"

#include "recover.h"
#include "log.h"

static recover_t *create_recover(void);	
static recover_data_t lend(recover_t *recover);
static int giveback(recover_t *recover, recover_data_t item);
static int destroy_recover(recover_t *recover);
static int add_size(recover_t *recover);
static int incr_recover(recover_t *recover);

static int inited = 0;
static recover_impl_t recover_impl;

recover_impl_t *recover_getinstance(void)
{
  recover_impl_t *ret = &recover_impl;	
  if(0 == inited)
  {
    ret->create_recover = create_recover;	
    ret->lend = lend;	
    ret->giveback = giveback;	
    ret->destroy_recover = destroy_recover;	
    ret->incr_recover = incr_recover;	
    inited = 1;	
  }
  return ret;
}

/**
 * @desc 创建一个回收池
 * @param void
 * @return recover_t *
 */
recover_t *create_recover(void)
{
  recover_t *recover = NULL;	
  DMALLOC(recover, recover_t *, sizeof(recover_t));
  if(NULL == recover) return NULL;
  recover->size = RECOVER_DEFAULT_SIZE;
  recover->curr_num = 0;
  recover->total = 0;
  DCALLOC(recover->data, recover_data_t *, recover->size, sizeof(recover_data_t));
  if(NULL == recover->data)
  {
    DFREE(recover);
    return NULL;
  }
  return recover;
}

/**
 * @desc 从回收池中借出一块内存
 * @param recover_t *recover 回收池句柄
 * @return recover_data_t 内存地址
 */
recover_data_t lend(recover_t *recover)
{
  assert(recover);	
  if(0 == recover->curr_num) return NULL;
  recover_data_t ret = NULL;
  ret = recover->data[--recover->curr_num];
  return ret;
}

/**
 * @desc 将一块内存归还到回收池中
 * @param recover_t *recover内存池
 * @param recover_data_t item 内存地址
 * @return int 0=成功，-1=失败
 */
int giveback(recover_t *recover, recover_data_t item)
{
  assert(recover && item);	
  int ret = -1;
  if(0 == add_size(recover))
  {
    recover->data[recover->curr_num++] = item;	
    if(recover->curr_num > recover->total) recover->total = recover->curr_num;
    ret = 0;
  }
  return ret;	
}

/**
 * @desc 销毁内存池
 * @param recover_t *recover 内存池句柄
 * @return int 0=成功，-1=失败
 */
int destroy_recover(recover_t *recover)
{
  assert(recover);	
  DFREE(recover->data);
  DFREE(recover);
  return 0;
}

/**
 * @desc 为内存池扩容
 * @param recover_t *recover 内存池句柄
 * @return 0=成功，-1=失败
 */
int add_size(recover_t *recover)
{
  assert(recover);	
  if(recover->curr_num == recover->size)
  {
    recover->size *= 2;
    DREALLOC(recover->data, recover_data_t *, recover->size * sizeof(recover_data_t));
    if(NULL == recover->data) return -1;
  }
  return 0;
}

/**
 * @desc 增加内存池的历史总容量
 * @param recover_t *recover 回收池句柄
 * @return 0=成功，-1=失败
 */
int incr_recover(recover_t *recover)
{
  assert(recover);	
  return ++recover->total;
}

