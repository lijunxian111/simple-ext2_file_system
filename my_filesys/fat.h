#ifndef _FAT_H_
#define _FAT_H_

#include "utils.h"
#include "types.h"
#include "disk.h"
#include<stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>


#define TOTAL_DISK_SIZE  2360832+MAX_BLOCK_SIZE
#define MAX_BLOCK_SIZE 512

#define MAX_NAME_LENGTH 256
#define MAX_EnTRY_NAME_LENGTH 11

#define DIRECT_BLOCKS 6
#define INODE_BLOCKS 8

#define NUM_OF_GROUPS 1
#define NUM_OF_BLOCKS 4611
#define NUM_OF_INODES 4096

#define INODE_SIZE 64
#define GROUP_SIZE BLOCKS_PER_GROUP*MAX_BLOCK_SIZE


#define BLOCKS_PER_GROUP 4096
#define INODES_PER_GROUP 4096

#define INODES_PER_BLOCK		(MAX_BLOCK_SIZE / INODE_SIZE)
#define ENTRY_PER_BLOCK			(MAX_BLOCK_SIZE / sizeof(EXT2_DIR_ENTRY))

#define DIR_ENTRY_FREE			0x01
#define DIR_ENTRY_NO_MORE		0x00
#define DIR_ENTRY_OVERWRITE		1

#define NUM_OF_ROOT_INODE		1

#ifdef _WIN32
#pragma pack(push,fatstructures)
#endif
#pragma pack(1)

typedef struct group
{ 
     char 	bg_volume_name[16];  	//卷名
     uint16   	bg_block_bitmap;       	//保存块位图的块号
     uint16   	bg_inode_bitmap;     	//保存索引结点位图的块号
     uint16	bg_inode_table;     	//索引结点表的起始块号
     uint16	bg_free_blocks_count;   	//本组空闲块的个数
     uint16	bg_free_inodes_count;   	//本组空闲索引结点的个数
     uint16	bg_used_dirs_count;	//本组目录的个数
     char	bg_pad[4]; 	               //填充(0xff)		
}ext2_group;

typedef struct { 
    uint16	inode;	//索引节点号
    uint16   rec_len;	//目录项长度
    uint8  name_len;        //文件名长度
    uint8   file_type;     //文件类型(1:普通文件，2:目录…)
    char	  name[256];    //文件名
}ext2_dir_entry ; 

typedef struct { 
    uint16	inode;	//索引节点号
    uint16   rec_len;	//目录项长度
    uint8  name_len;        //文件名长度
    uint8   file_type;     //文件类型(1:普通文件，2:目录…)
    char	  name[16];    //文件名
    char          content[490];
}ext2_file;    //普通文件 


typedef struct{
     uint16     i_mode;     	//文件类型及访问权限
     uint16	i_blocks; 	//文件的数据块个数
     uint32	i_size;     	                //大小(字节)
     uint64   	i_atime;   	//访问时间
     uint64   	i_ctime;   	//创建时间
     uint64  	i_mtime;   	//修改时间
     uint64   	i_dtime;    	//删除时间
     uint16 	i_block[8];   	//指向数据块的指针
     char	i_pad[8];	                //填充1(0xff)		
}ext2_inode;


typedef struct
{
	ext2_inode inode[4096];
}ext2_inode_table;

typedef struct
{
	bool block_bitmap[4096]; 
}ext2_block_bitmap;

typedef struct
{
	bool inode_bitmap[4096]; 
}ext2_inode_bitmap;

typedef struct{
       ext2_group gp;
       //ext2_block_bitmap b_bit;
       //ext2_inode_bitmap i_bit;
       ext2_inode_table i_tb;
       
}ext2_file_system;

typedef struct
{
	uint32 group;
	uint32 block;
	uint32 offset;
}ext2_dir_entry_location;

typedef struct
{
	ext2_file_system *fs;
        int entry_length;

	ext2_dir_entry *entry;
	ext2_dir_entry_location *location;
}ext2_node;

#ifdef _WIN32
#pragma pack(pop, fatstructures)
#else
#pragma pack()
#endif

//functions

int update_group_desc(ext2_node *);
int reload_group_desc(ext2_node *);
int init_root(ext2_node *, ext2_inode *);
int create_root_entry(ext2_node *);
int update_block_map(ext2_node *, int);//更新位图，数据块
int update_inode_map(ext2_node *, int);//更新位图，索引结点
int alloc_new_inode(ext2_inode *, int);
int alloc_new_block(ext2_inode *,int);
int read_inode(ext2_inode*, int, FILE *);
int del_inode(int, FILE *);
int write_inode(ext2_inode*, int, FILE *);
int write_dir(ext2_dir_entry *, FILE *);
uint64 gettime();

#endif
