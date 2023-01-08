#ifndef _CMD_H_
#define _CMD_H_

#include<stdio.h>
#include<stdlib.h>
#include<memory.h>

#include"utils.h"
#include"fat.h"
#include"disk.h"
#include"types.h"

int shell_cmd_exit(void);
int exit_my(void);
int shell_cmd_ls(void);
int shell_cmd_mkdir(void);//创建目录
int shell_cmd_cd(void);
int shell_cmd_rmdir(void);
int shell_cmd_create(void);//创建文件
int shell_cmd_delete(void);//删除文件
int shell_cmd_open(void);
int shell_cmd_close(void);
char* match(char *, const char *);
int del_substr(char *, const char *);


#endif
