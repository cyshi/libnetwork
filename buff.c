#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "dalloc.h"
#include "buff.h"
#include "log.h"

/**
 * @desc 创建一个buff
 * @param int length 长度
 * @return buff_t *
 */
buff_t *create_buff(int length)
{
  assert(length>=0);
  if(length < BUFF_MIN_LENGTH) length = BUFF_MIN_LENGTH;	
  buff_t *buff = NULL;
  DMALLOC(buff,buff_t *,sizeof(buff_t));
  if(NULL == buff)
  {
    log_error("create_buff malloc fail");
    return NULL;
  }
  buff->length = 0;
  buff->max_length = length;
  DCALLOC(buff->data,void *,buff->max_length,sizeof(char));
  if(NULL == buff->data)
  {
    log_error("create_buff calloc fail");
    DFREE(buff);
    return NULL;
  }
  return buff;
}

/**
 * @desc 添加内容到buff
 * @param buff_t *buff buff
 * @param void *data 数据
 * @param int length 长度
 * @return int 0=成功，-1=失败
 */
int append_buff(buff_t *buff,void *data,int length)
{
  assert(buff && data);	
  if(buff->length + length > buff->max_length)
  {
    while(buff->max_length < buff->length + length) buff->max_length *= 2;
    DREALLOC(buff->data,void *,buff->max_length*sizeof(char));
    if(NULL == buff->data)
    {
      log_error("append_buff realloc fail");
      return -1;
    }
  }
  memcpy(buff->data+buff->length,data,length);
  buff->length += length;
  return 0;
}

/**
 * @desc 添加内容到buff
 * @param buff_t *buff buff
 * @param void *data 数据
 * @param int length 长度
 * @return int 0=成功，-1=失败
 */
int prepend_buff(buff_t *buff,void *data,int length)
{
  assert(buff && data);	
  if(buff->length + length > buff->max_length)
  {
    while(buff->max_length < buff->length + length) buff->max_length *= 2;
    DREALLOC(buff->data,void *,buff->max_length*sizeof(char));
    if(NULL == buff->data)
    {
      log_error("prepend_buff realloc error");
      return -1;
    }
  }
  memmove(buff->data+length,buff->data,buff->length);
  memcpy(buff->data,data,length);
  buff->length += length;
  return 0;
}

/**
 * @desc 删除buff
 * @param buff_t *buff buff
 * @return void
 */
void destroy_buff(buff_t *buff)
{
  assert(buff);
  DFREE(buff->data);	
  DFREE(buff);
}

/**
 * @desc 将buff的length设置成0，当原buff的max_length大于RESET_FREE_LENGTH,我们将进行realloc操作
 * @param buff_t *buff
 * @return int
 */
int reset_buff(buff_t *buff)
{
  assert(buff);
  if(buff->max_length > RESET_FREE_LENGTH)
  {
    //释放部分内存
    DREALLOC(buff->data, void *, RESET_FREE_LENGTH);		
    if(NULL == buff->data)
    {
      log_error("reset_buff realloc fail");
      return -1;
    }
    buff->max_length = RESET_FREE_LENGTH;
  }
  buff->length = 0;	
  return 0;
}

/**
 * @desc 扩展buff的max_length,不改变length
 * @param buff_t *buff 
 * @param int size 增加的大小
 * @return int 0=成功，-1=失败
 */
int expand_buff(buff_t *buff, int size)
{
  int target_length = buff->max_length + size;	
  while(buff->max_length < target_length) buff->max_length *= 2;
  DREALLOC(buff->data, void *, sizeof(char) * buff->max_length);
  if(NULL == buff->data)
  {
    log_error("expand_buff realloc fail");
    return -1;
  }
  else return 0;
}

