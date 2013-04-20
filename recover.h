#ifndef _RECOVER_H
#define _RECOVER_H

#define RECOVER_DEFAULT_SIZE 10

typedef void * recover_data_t;
typedef struct recover_impl_s recover_impl_t;
typedef struct recover_s recover_t;

struct recover_s
{
  int total; //历史以来的最大总数	
  int size; //内存的总数
  int curr_num; //当前还剩余多少
  recover_data_t *data; //顺序表，实现了一个stack
};

//对外接口
struct recover_impl_s
{
  recover_t *(*create_recover)(void);	
  recover_data_t (*lend)(recover_t *recover);
  int (*giveback)(recover_t *recover, recover_data_t item);
  int (*destroy_recover)(recover_t *recover);
  int (*incr_recover)(recover_t *recover);
};

recover_impl_t *recover_getinstance(void);
#endif
