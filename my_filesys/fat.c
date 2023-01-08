#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<time.h>

#include "disk.h"
#include "fat.h"

#define MIN(a, b)                    ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX(a, b)                    ( ( a ) > ( b ) ? ( a ) : ( b ) )



//time function

uint64 gettime()// 获取当前时间
{
    time_t time_p;
    struct tm *p;
    char time1[28];
    time(&time_p);
    p=gmtime(&time_p);
    int minute=p->tm_min;
    int hour=8+p->tm_hour;
    int day=p->tm_mday;
    int mon=p->tm_mon+1;
    int year=1900+p->tm_year;
    sprintf(time1,"%d%d%d%d%d",year,mon,day,hour,minute);
    return (uint64)atoi(time1);
}

int write_dir(ext2_dir_entry *lst, FILE *fp)//写目录
{
    fwrite(&lst[0].inode,sizeof(uint16),1,fp);
    fwrite(&lst[0].rec_len,sizeof(uint16),1,fp);
    fwrite(&lst[0].name_len,sizeof(uint8),1,fp);
    fwrite(&lst[0].file_type,sizeof(uint8),1,fp);
    fwrite(lst[0].name,sizeof(char),strlen(lst[0].name)+1,fp);
    
    fwrite(&lst[1].inode,sizeof(uint16),1,fp);
    fwrite(&lst[1].rec_len,sizeof(uint16),1,fp);
    fwrite(&lst[1].name_len,sizeof(uint8),1,fp);
    fwrite(&lst[1].file_type,sizeof(uint8),1,fp);
    fwrite(lst[1].name,sizeof(char),strlen(lst[1].name)+1,fp);
    rewind(fp);
    return 0;
}

int write_inode(ext2_inode* node, int pos, FILE *fp)  //写索引结点
{
    fseek(fp,(long)pos,0);
    fwrite(&node->i_mode,sizeof(uint16),1,fp);
    fwrite(&node->i_blocks,sizeof(uint16),1,fp);
    fwrite(&node->i_size,sizeof(uint32),1,fp);
    fwrite(&node->i_atime,sizeof(uint64),1,fp);
    fwrite(&node->i_ctime,sizeof(uint64),1,fp);
    fwrite(&node->i_mtime,sizeof(uint64),1,fp);
    fwrite(&node->i_dtime,sizeof(uint64),1,fp);
   
    fwrite(node->i_block,sizeof(uint16),8,fp);
    
    fwrite(node->i_pad,sizeof(char),8,fp);
    return 0;
}

int del_inode(int dir_2_rm, FILE *fp)//删除某个索引结点
{
    int pos=3*(int)MAX_BLOCK_SIZE+64*(int)(dir_2_rm-1);
    fseek(fp,(long)pos,0);
    char c[1]={0};
    fwrite(c,sizeof(char),64,fp);
    return 0;
}

// format instructions %hd short %d int %ld long

int update_group_desc(ext2_node *parent)
{
    FILE *fp=fopen("FS.bin","rb+");
    if(fp==NULL)
    {
        return -1;
    }
    
    fwrite(parent->fs->gp.bg_volume_name,1,16,fp);
    fwrite(&parent->fs->gp.bg_block_bitmap,sizeof(uint16),1,fp);
    //fprintf(fp,"%hd",parent->fs->gp.bg_block_bitmap);
    fwrite(&parent->fs->gp.bg_inode_bitmap,sizeof(uint16),1,fp);
    //fprintf(fp,"%hd",parent->fs->gp.bg_inode_bitmap);
    fwrite(&parent->fs->gp.bg_inode_table,sizeof(uint16),1,fp);
    //fprintf(fp,"%hd",parent->fs->gp.bg_inode_table);
    fwrite(&parent->fs->gp.bg_free_blocks_count,sizeof(uint16),1,fp);
    //fprintf(fp,"%hd",parent->fs->gp.bg_free_blocks_count);
    fwrite(&parent->fs->gp.bg_free_inodes_count,sizeof(uint16),1,fp);
    //fprintf(fp,"%hd",parent->fs->gp.bg_free_inodes_count);
    fwrite(&parent->fs->gp.bg_used_dirs_count,sizeof(uint16),1,fp);
    //fprintf(fp,"%hd",parent->fs->gp.bg_used_dirs_count);
    
    fwrite(parent->fs->gp.bg_pad,sizeof(char),4,fp);
    
    rewind(fp);
    fclose(fp);
    return 0;
}

int reload_group_desc(ext2_node *parent)
{
    return 0;
}

int init_root(ext2_node *parent, ext2_inode *root_inode)
{
    BLOCK pos=3*MAX_BLOCK_SIZE;  //根结点的索引结点从块3开始写入
    root_inode=(ext2_inode *)malloc(sizeof(ext2_inode));
    root_inode->i_mode=2;
    root_inode->i_blocks=1; //".", ".."占一个数据块
    root_inode->i_size=64;
    root_inode->i_atime=0;
    root_inode->i_ctime=gettime();
    //printf("%ld\n",root_inode->i_ctime);
    root_inode->i_mtime=0;
    root_inode->i_dtime=0;
    for(int i=0;i<8;i++)
    {
        root_inode->i_block[i]=0; //从位置3开始的绝对位置
    }
    root_inode->i_block[0]=512;
    //root_inode->i_block[1]=1+512;
    memset(root_inode->i_pad,'a',8);
    FILE *fp=fopen("FS.bin","rb+");
    if(fp==NULL)
    {
        return -1;
    }
    write_inode(root_inode,pos,fp);
    
    rewind(fp);
    fclose(fp);
    return 0;
}

int create_root_entry(ext2_node *parent)
{
    if(!parent || !parent->fs)
    {
        return -1;
    }
    parent->entry=(ext2_dir_entry *)malloc(parent->entry_length*sizeof(ext2_dir_entry));
    parent->location=(ext2_dir_entry_location *)malloc(parent->entry_length*sizeof(ext2_dir_entry_location));
    BLOCK pos=512+1+1+1;
    parent->entry[0].inode=1;
    parent->entry[0].rec_len=8;
    parent->entry[0].name_len=1;
    parent->entry[0].file_type=2;
    strcpy(parent->entry[0].name,".");
    parent->location[0].group=1;
    parent->location[0].block=516;
    parent->location[0].offset=0;
    parent->location[1].group=1;
    parent->location[1].block=516;
    parent->location[1].offset=parent->entry[0].rec_len;
    parent->entry[1].inode=1;
    parent->entry[1].rec_len=9;
    parent->entry[1].name_len=2;
    parent->entry[1].file_type=2;
    strcpy(parent->entry[1].name,"..");
    FILE *fp=fopen("FS.bin","rb+");
    if(fp==NULL)
    {
        return -1;
    }

    fseek(fp,(long)pos*MAX_BLOCK_SIZE,0);
    write_dir(parent->entry,fp);
    
    rewind(fp);
    fclose(fp);
}

int update_block_map(ext2_node *parent, int alloc_block)
{
    FILE *fp=fopen("FS.bin","rb+");
     if(fp==NULL)
    {
        return -1;
    }
    fseek(fp,(long)parent->fs->gp.bg_block_bitmap*MAX_BLOCK_SIZE,0);
    bool tmp=0;
    for(int i=0;i<alloc_block;i++)
    {
        fwrite(&tmp,sizeof(bool),1,fp);
    }
    tmp=1;
    fwrite(&tmp,sizeof(bool),1,fp);
    
    rewind(fp);
    fclose(fp);
    return 0;
}

int update_inode_map(ext2_node *parent, int alloc_inode)
{
    FILE *fp=fopen("FS.bin","rb+");
     if(fp==NULL)
    {
        return -1;
    }
    fseek(fp,(long)parent->fs->gp.bg_inode_bitmap*MAX_BLOCK_SIZE,0);
    bool tmp=0;
    for(int i=1;i<alloc_inode;i++)  //索引结点从1开始计数
    {
        fwrite(&tmp,sizeof(bool),1,fp);
    }
    tmp=1;
    fwrite(&tmp,sizeof(bool),1,fp);
    rewind(fp);
    fclose(fp);
    return 0;
}

int alloc_new_inode(ext2_inode *new_node, int last_pos)
{
   ext2_inode_bitmap i_bit;
   FILE *fp=fopen("FS.bin","rb+");
   if(fp==NULL)
   {
        return -1;
   }
   int alloc_pos=-1;
   fseek(fp,(long)2*MAX_BLOCK_SIZE,0);
   fread(i_bit.inode_bitmap,sizeof(bool),4096,fp);
   rewind(fp);
   for(int i=last_pos-1;i<4096;i++)
   {
       if(i_bit.inode_bitmap[i]==(bool)0)
       {
           alloc_pos=i+1;
           i_bit.inode_bitmap[i]=1;
           break;
       }
   }
   
   //更新索引结点位图
   fseek(fp,(long)2*MAX_BLOCK_SIZE,0);
   fwrite(i_bit.inode_bitmap,sizeof(bool),4096,fp);
   
   rewind(fp);
   fclose(fp);
   return alloc_pos;
}

int read_inode(ext2_inode* tmp_inode, int current_dir, FILE *fp)
{
    int pos=3*(int)MAX_BLOCK_SIZE+64*(int)(current_dir-1);
    fseek(fp,(long)pos,0);

    // TODO: modify access time,...
    fread(&tmp_inode->i_mode,sizeof(uint16),1,fp);
    fread(&tmp_inode->i_blocks,sizeof(uint16),1,fp);
    fread(&tmp_inode->i_size,sizeof(uint32),1,fp);
    fread(&tmp_inode->i_atime,sizeof(uint64),1,fp);
    fread(&tmp_inode->i_ctime,sizeof(uint64),1,fp);
    fread(&tmp_inode->i_mtime,sizeof(uint64),1,fp);
    fread(&tmp_inode->i_dtime,sizeof(uint64),1,fp);
   
    fread(tmp_inode->i_block,sizeof(uint16),8,fp);
    
    fread(tmp_inode->i_pad,sizeof(char),8,fp);

    //修改访问时间
    fseek(fp,(long)pos+8,0);//跳过前面8字节
    tmp_inode->i_atime=gettime();
    fwrite(&tmp_inode->i_atime,sizeof(uint64),1,fp);
}

int alloc_new_block(ext2_inode *node,int last_pos)
{
    ext2_block_bitmap b_bit;
    FILE *fp=fopen("FS.bin","rb+");
    if(fp==NULL)
    {
        return -1;
    }
    int alloc_pos=-1;
    fseek(fp,(long)1*MAX_BLOCK_SIZE,0);
    fread(b_bit.block_bitmap,sizeof(bool),4096,fp);
    rewind(fp);
    for(int i=last_pos;i<4096;i++)
    {
       if(b_bit.block_bitmap[i]==(bool)0)
       {
           alloc_pos=i;
           b_bit.block_bitmap[i]=1;
           break;
       }
    }
    for(int i=0;i<8;i++)
    {
        if(node->i_block[i]==0)
        {
            node->i_block[i]=alloc_pos+512;
            break;
        }
    }
    fseek(fp,(long)MAX_BLOCK_SIZE,0);
    fwrite(b_bit.block_bitmap,sizeof(bool),4096,fp);
    rewind(fp);
    fclose(fp);
    return alloc_pos+512;
}
