#ifndef _DISK_H_
#define _DISK_H_
#include "utils.h"
#include<string.h>

int disk_init(BLOCK,unsigned int);  //磁盘初始化

int disk_uinit();  //磁盘空白化

int disk_read(BLOCK, void* );  //磁盘读

int disk_write(BLOCK, const void *);  //磁盘写


#endif
