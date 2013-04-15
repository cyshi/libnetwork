#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>
#include "buff.h"

int setnonblocking(int fd);
int setreuseaddr(int fd);
int setnodelay(int fd);
int setotheropt(int fd);
uint64_t get_curr_time(void);
int is_file(const char *filename);
int is_dir(const char *dirname);
double fytime(void);
double ttgetloadavg(void);
#endif
