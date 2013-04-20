#ifndef _BUFF_H
#define _BUFF_H

#define BUFF_MIN_LENGTH 1024
#define RESET_FREE_LENGTH 512000 //500K 当buff原来的max_length大于500k我们将会对原内存做realloc缩小操作,不过从glibc我们知道他是根本不会释放复用的！

typedef struct buff_s buff_t;
//这个buff是2进制安全的
struct buff_s
{
  void *data;
  int max_length;//最大长度
  int length;//当前长度
};

buff_t *create_buff(int length);//创建一个buff
int append_buff(buff_t *buff,void *data,int length);//添加内容到buff后面
int prepend_buff(buff_t *buff,void *data,int length);//添加内容到buff前面
void destroy_buff(buff_t *buff);//删除buff
int reset_buff(buff_t *buff); //将buff的长度设置成0
int expand_buff(buff_t *buff, int size); //将buff的max_length扩大
#endif
