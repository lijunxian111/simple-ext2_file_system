#pragma warning(disable : 4995)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include<unistd.h>
#include<stdbool.h>

#include"utils.h"
#include"fat.h"
#include"disk.h"
#include"types.h"
#include"cmd.h"

#define		BLOCK_SIZE				512
#define		NUMBER_OF_SECTORS		((4612*512) / BLOCK_SIZE)

#define COND_MOUNT				0x01
#define COND_UMOUNT				0x02
#define CMD_NUM                                 7   //之后修改！



typedef struct
{
	char	name[15];
	int(*handler)(void);
	char	conditions;
}COMMAND;

static COMMAND commands_lst[]=
{
     // { "mkdir",	shell_cmd_mkdir,	COND_MOUNT	},
      {"exit",          shell_cmd_exit,         COND_MOUNT      },
      {"ls",             shell_cmd_ls,         COND_MOUNT      },
      {"mkdir",             shell_cmd_mkdir,         COND_MOUNT      },
      {"cd",             shell_cmd_cd,         COND_MOUNT      },
      {"rmdir",          shell_cmd_rmdir,      COND_MOUNT      },
      {"create",          shell_cmd_create,     COND_MOUNT      },
      {"delete",          shell_cmd_delete,     COND_MOUNT      },
      {"open",          shell_cmd_open,     COND_MOUNT      },
      {"close",          shell_cmd_close,     COND_MOUNT      }
};

ext2_node *PARENT;   //文件系统
ext2_inode *root_inode;  //根结点

char cur_condition=COND_UMOUNT;
uint16 fopen_table[16] ; /*文件打开表，最多可以同时打开16个文件*/
uint16 last_alloc_inode; /*上次分配的索引结点号*/
uint16 last_alloc_block; /*上次分配的数据块号*/
uint16 current_dir ;  /*当前目录(索引结点）*/
char current_path[256];   	 /*当前路径(字符串) */

void work(void);
void quit(void);
void restart();
void unknown_cmd();

int main(int argc, char *argv[])
{
    memset(current_path,0,256);
    memset(fopen_table,0,sizeof(fopen_table));
    BLOCK block=4611+1;
    char usercmd[14]={'\0'};
    int flag=1;
    if(disk_init(block,512)<0)
    {
        printf("File System Initialization failed!\n");
        return -1;
    }
    printf("Welcome to Mr Li's File System. shell, format or quit to choose. \n");
    while(flag==1)  //循环运行
    {
       printf("choose usercmd: ");
       scanf("%s",usercmd);
       getchar();
       //printf("111\n");
       if(strcmp(usercmd,"shell")==0)
       {
            work();
       }
       else if(strcmp(usercmd,"format")==0)
       {
            cur_condition=COND_MOUNT;
            PARENT=(ext2_node*)malloc(sizeof(ext2_node));//分配空间很重要！
            PARENT->fs=(ext2_file_system*)malloc(sizeof(ext2_file_system));
            PARENT->entry_length=2;//目前两个目录项
            last_alloc_inode=1;
            last_alloc_block=0;//写目录项
            current_dir=1;
            strcpy(current_path,"root");
            //printf("111\n");
            restart();
       }
       else if(strcmp(usercmd,"quit")==0){
            cur_condition=COND_UMOUNT;
            quit();
            flag=0;
       }
    }
    if(PARENT->fs)
    {
          free(PARENT->fs);
    }
    if(PARENT)
    {
          free(PARENT);
    }
    if(root_inode)
    {
        free(root_inode);
    }
    return 0;
}


void work()
{
    int flag2=0;
    bool deal=0; //判断命令是否执行
    while(flag2==0)
    {
        char cmd[10]={0};
        deal=0; //这里注意更新
        if(strcmp(current_path,"root")==0)
        {
             printf("~$: ");    
        }
        else
        {
             printf("~/%s$: ",current_path);
        }
        scanf("%s",cmd);
        //getchar();
        for(int i=0;i<CMD_NUM;i++)
        {
            if(strcmp(commands_lst[i].name,cmd)==0)
            {
                if(cur_condition!=commands_lst[i].conditions) break;
                flag2=commands_lst[i].handler();
                deal=1;
                //printf("111\n");
            }
            if(i==CMD_NUM-1 && deal==0)
            {
               unknown_cmd();
            }
        }
    }
    return;
}

void restart()
{
    //printf("111");
    sleep(1);
    printf("Name your volume: ");
    scanf("%s",PARENT->fs->gp.bg_volume_name);
    getchar();
    printf("%s\n",PARENT->fs->gp.bg_volume_name);
    
    //memset(PARENT->fs->b_bit.block_bitmap,MAX_BLOCK_SIZE,0);
    //memset(PARENT->fs->i_bit.inode_bitmap,MAX_BLOCK_SIZE,0);
    PARENT->fs->gp.bg_block_bitmap=1;
    PARENT->fs->gp.bg_inode_bitmap=2;
    PARENT->fs->gp.bg_inode_table=3;
    PARENT->fs->gp.bg_free_blocks_count=4096;
    PARENT->fs->gp.bg_free_inodes_count=4096;
    PARENT->fs->gp.bg_used_dirs_count=0;
    strcpy(PARENT->fs->gp.bg_pad,"aaaa");
    if(update_group_desc(PARENT)==0)
    {
        printf("volume: ");
        printf("%s\n",PARENT->fs->gp.bg_volume_name);
        printf("MAX SPACE: 2 MiB\n");
        printf("ALLOCATED: %f MiB\n",(float)(4096-PARENT->fs->gp.bg_free_blocks_count)/2048.0);
        printf("AVAILABLE SPACE: %f MiB\n",(float)PARENT->fs->gp.bg_free_blocks_count/2048.0);
        init_root(PARENT,root_inode);
        printf("root dir created!\n");
        
        update_block_map(PARENT, last_alloc_block);
        //update_block_map(PARENT, last_alloc_block);  //0和1号数据块已经分配
        update_inode_map(PARENT, last_alloc_inode);
        //PARENT->fs->b_bit.block_bitmap[1]=1;
        //PARENT->fs->i_bit.inode_bitmap[0]=1;
        create_root_entry(PARENT);
        PARENT->fs->gp.bg_free_blocks_count--;
        PARENT->fs->gp.bg_free_inodes_count--;  //分配了一个数据块和一个索引结点
        PARENT->fs->gp.bg_used_dirs_count++;
        update_group_desc(PARENT);
    }
    else
    {
        printf("format failed!\n");
    }
    
    return;
}

void quit(void)
{
    disk_uinit();
    printf("Goodbye and Good luck!\n");
    return;
}


//instructions

void unknown_cmd(void)
{
    printf("ls -cmds:\n");
    for(int i=0;i<CMD_NUM;i++)
    {
        printf("%s\n",commands_lst[i].name);
    }
    printf("\n");
    return;
}

int shell_cmd_exit(void)
{
    sleep(0.5);
    return 1;
}

int shell_cmd_ls(void)
{
    FILE *fp=fopen("FS.bin","rb+");
    if(!fp) return -1;
    ext2_inode *tmp_inode=(ext2_inode *)malloc(sizeof(ext2_inode));
    read_inode(tmp_inode,current_dir,fp);  //读入当前索引结点

    ext2_dir_entry *entry_list=(ext2_dir_entry *)malloc(16*sizeof(ext2_dir_entry));
    int count=0;

    // TODO:i_block[i]<512
    for(int i=0;i<tmp_inode->i_blocks;i++)
    {
        if(tmp_inode->i_block[i]>=512)
        {
             //printf("111\n");
             fseek(fp,(long)(tmp_inode->i_block[i]+3)*MAX_BLOCK_SIZE,0);
             int read_flag=1;
             while(read_flag==1)
            {
                 fread(&entry_list[count].inode,sizeof(uint16),1,fp);
                 //printf("%hd\n",entry_list[count].inode);
                 if(entry_list[count].inode==0) break;
                 fread(&entry_list[count].rec_len,sizeof(uint16),1,fp);
                 fread(&entry_list[count].name_len,sizeof(uint8),1,fp);
                 fread(&entry_list[count].file_type,sizeof(uint8),1,fp);
                 if(entry_list[count].file_type>0)  //判断不要读入NULL！
                 {
                     fread(entry_list[count].name,sizeof(char),entry_list[count].rec_len-6,fp);
                 }
                 //printf("%d\n",entry_list[count].inode);
                 count++;
            }
        }
        else{
            continue;
        }
    }
  
    //printf("%d\n",count);
    //printf("%c\n",tmp_inode->i_pad[0]); 测试成功
    for(int i=0;i<count;i++)
    {
         if(entry_list[i].file_type>0)
         {
             printf("%s ",entry_list[i].name);
         }
    }
    printf("\n");
    

    if(tmp_inode)
    {
        free(tmp_inode);
    }
    if(entry_list)
    {
        free(entry_list);
    }
    rewind(fp);
    fclose(fp);
    //printf("111\n");
    return 0;
}


int shell_cmd_mkdir(void)
{
    char dir_name[20];
    rewind(stdin);
    scanf("%s",dir_name);
    //getchar();
    
    FILE *fp=fopen("FS.bin","rb+");
    //printf("%s\n",dir_name);
    if(!fp) return -1;
    ext2_inode *tmp_inode=(ext2_inode *)malloc(sizeof(ext2_inode));
    ext2_inode *new_inode=(ext2_inode *)malloc(sizeof(ext2_inode));
    read_inode(tmp_inode,current_dir,fp);//读入当前索引结点
    int pos=3*(int)MAX_BLOCK_SIZE+64*(int)(current_dir-1);
    //printf("%ld\n",tmp_inode->i_ctime);
    tmp_inode->i_atime=gettime();
    write_inode(tmp_inode,pos,fp);

    bool block_bitmap[4096];
    bool inode_bitmap[4096];
    fseek(fp,MAX_BLOCK_SIZE,0);
    fread(block_bitmap,sizeof(bool),4096,fp);
    fseek(fp,MAX_BLOCK_SIZE*2,0);
    fread(inode_bitmap,sizeof(bool),4096,fp);
    
    /*新结点
    */
    new_inode->i_mode=2;
    new_inode->i_blocks=1;
    new_inode->i_size=64;
    new_inode->i_atime=0;
    new_inode->i_ctime=gettime();
    new_inode->i_mtime=0;
    new_inode->i_dtime=0;
    
    for(int i=0;i<8;i++)
    {
         new_inode->i_block[i]=0;
    }
    memset(new_inode->i_pad,'a',8);
    //TODO:分配数据块


    ext2_dir_entry entry;
    ext2_dir_entry new_entry;
    memset(new_entry.name,0,256);

    //int count=0;
    
    int count=0;
    // TODO:i_block[i]<512
    for(int i=0;i<tmp_inode->i_blocks;i++)
    {
        if(tmp_inode->i_block[i]>=512)
        {
             
             fseek(fp,(long)(tmp_inode->i_block[i]+3)*MAX_BLOCK_SIZE,0);
             int read_flag=1;
             while(read_flag==1)
            {
                 fread(&entry.inode,sizeof(uint16),1,fp);
                 if(entry.inode==(uint16)0) break;
                 fread(&entry.rec_len,sizeof(uint16),1,fp);
                 
                 fread(&entry.name_len,sizeof(uint8),1,fp);
                 fread(&entry.file_type,sizeof(uint8),1,fp);
                 if(entry.rec_len>0)  //判断不要读入NULL！
                 {
                     fread(entry.name,sizeof(char),entry.rec_len-6,fp);
                 }
                 
                 count+=entry.rec_len;
                 //printf("%d\n",entry_list[count].inode);
                
            }
        }
        else{
            continue;
        }
    }
    
    if(entry.inode==0)
    {
        fseek(fp,-2L,1);
    }
    
    
    //分配目录数据块
    int alloc_block=alloc_new_block(new_inode,last_alloc_block);
    last_alloc_block=alloc_block;
    block_bitmap[alloc_block]=(bool)1;
    //printf("%d\n",alloc_block);

    int alloc_pos=alloc_new_inode(new_inode,last_alloc_inode);
    inode_bitmap[alloc_pos-1]=(bool)1;

    int new_pos=3*(int)MAX_BLOCK_SIZE+64*(int)(alloc_pos-1);
    FILE *fp2=fopen("FS.bin","rb+");
    write_inode(new_inode,new_pos,fp2);
    fclose(fp2);
   //写索引结点

    new_entry.inode=alloc_pos;
    new_entry.rec_len=6+strlen(dir_name)+1;
    new_entry.name_len=strlen(dir_name);
    new_entry.file_type=2;
    strcpy(new_entry.name,dir_name);
   

    fwrite(&new_entry.inode,sizeof(uint16),1,fp);
    fwrite(&new_entry.rec_len,sizeof(uint16),1,fp);
    fwrite(&new_entry.name_len,sizeof(uint8),1,fp);
    fwrite(&new_entry.file_type,sizeof(uint8),1,fp);
    fwrite(new_entry.name,sizeof(char),strlen(dir_name)+1,fp);
    
    //更新组描述符信息
    PARENT->fs->gp.bg_free_blocks_count--;
    PARENT->fs->gp.bg_free_inodes_count--;
    PARENT->fs->gp.bg_used_dirs_count++;
    PARENT->entry_length++;
    update_group_desc(PARENT);
    
    fseek(fp,(long)(alloc_block+3)*MAX_BLOCK_SIZE,0);
    ext2_dir_entry *new_entry_list=(ext2_dir_entry *)malloc(2*sizeof(ext2_dir_entry));
    new_entry_list[0].inode=alloc_pos;
    new_entry_list[0].rec_len=8;
    new_entry_list[0].name_len=1;
    new_entry_list[0].file_type=2;
    strcpy(new_entry_list[0].name,".");
    new_entry_list[1].inode=last_alloc_inode;
    new_entry_list[1].rec_len=9;
    new_entry_list[1].name_len=2;
    new_entry_list[1].file_type=2;
    strcpy(new_entry_list[1].name,"..");
    last_alloc_inode=(uint16)alloc_pos;//这里再更改否则写目录会出错
    write_dir(new_entry_list,fp);
    
    //更新位图
    fseek(fp,MAX_BLOCK_SIZE,0);
    fwrite(block_bitmap,sizeof(bool),4096,fp);
    fseek(fp,MAX_BLOCK_SIZE*2,0);
    fwrite(inode_bitmap,sizeof(bool),4096,fp);

    if(tmp_inode)
    {
        free(tmp_inode);
    }
    if(new_inode)
    {
        free(new_inode);
    }
    if(new_entry_list)
    {
        free(new_entry_list);
    }
    rewind(fp);
    fclose(fp);
    //printf("%s\n",dir_name);
    return 0;
}

int shell_cmd_cd(void)
{
    char dir_name[20];
    rewind(stdin);
    scanf("%s",dir_name);
    if(strcmp(dir_name,".")==0)
    {
        return 0;
    }
    
    FILE *fp=fopen("FS.bin","rb+");
    //printf("%s\n",dir_name);
    if(!fp) return -1;
    ext2_inode *tmp_inode=(ext2_inode *)malloc(sizeof(ext2_inode));
    read_inode(tmp_inode,current_dir,fp);
    int pos=3*(int)MAX_BLOCK_SIZE+64*(int)(current_dir-1);
    //printf("%ld\n",tmp_inode->i_ctime);
    tmp_inode->i_atime=gettime();
    write_inode(tmp_inode,pos,fp);
    ext2_dir_entry entry;
    ext2_dir_entry tmp_entry;
    int read_flag=1;
    int count=0;
    int deal=0;

    for(int i=0;i<tmp_inode->i_blocks;i++)
    {
        if(deal==1) break;
        if(tmp_inode->i_block[i]>=512)
        {
             
             fseek(fp,(long)(tmp_inode->i_block[i]+3)*MAX_BLOCK_SIZE,0);
             int read_flag=1;
             while(read_flag==1)
            {
                 fread(&entry.inode,sizeof(uint16),1,fp);
                 if(entry.inode==(uint16)0) 
                 {
                      printf("ERROR: [%s]No such file or directory!\n",dir_name);
                      if(tmp_inode)
                      {
                            free(tmp_inode);
                      }
                      return 0;
                 }
                 fread(&entry.rec_len,sizeof(uint16),1,fp);
                 fread(&entry.name_len,sizeof(uint8),1,fp);
                 fread(&entry.file_type,sizeof(uint8),1,fp);
                 if(entry.rec_len>0)  //判断不要读入NULL！
                 {
                     fread(entry.name,sizeof(char),entry.rec_len-6,fp);
                 }
                 
                 if(strcmp(entry.name,dir_name)==0)
                 {
                      if(entry.file_type!=2)
                      {
                           printf("ERROR: [%s] is not a directory!\n",dir_name);
                           if(tmp_inode)
                           {
                            free(tmp_inode);
                           }
                           return 0;
                      }
                      else
                      {
                          deal=1;
                          break;
                      }
                 }
                 count+=entry.rec_len;
                 //printf("%d\n",entry_list[count].inode);
                
            }
        }
        else{
            continue;
        }
    }
    current_dir=entry.inode;
    if(strcmp(entry.name,"..")==0)
    {
        if(strcmp(current_path,"root")==0) return 0;
        else
        {
            char *ptr=current_path;
            //printf("%s\n",current_path);
            ptr+=(strlen(current_path)-1);
            while(ptr>current_path)
            {
                if(*ptr=='/') break;
                ptr--;
            }
            char tp_char[256];
            char *tt=tp_char,*q=current_path;
            if(q==ptr) strcpy(current_path,"root");
          else
          {
            while(q<ptr)
            {
                //printf("%c",*tt);
                *tt=*q;
                q++;
                tt++;
            }
            *tt='\0';
            //printf("%s\n",tp_char);
            strcpy(current_path,tp_char);
          }
        }
    }
    else
    {
         if(strcmp(current_path,"root")==0)
         {
             strcpy(current_path,dir_name);
         }
         else
        {
         strcat(current_path,"/");
         strcat(current_path,dir_name);
       }
    }
    
    //TODO: modify access time
    if(tmp_inode)
    {
        free(tmp_inode);
    }
    return 0;
}


int shell_cmd_rmdir(void)
{
    char dir_name[20];
    rewind(stdin);
    scanf("%s",dir_name);
    //getchar();
    
    FILE *fp=fopen("FS.bin","rb+");
    //printf("%s\n",dir_name);
    if(!fp) return -1;
    
    ext2_inode *tmp_inode=(ext2_inode *)malloc(sizeof(ext2_inode));
    ext2_inode *new_inode=(ext2_inode *)malloc(sizeof(ext2_inode));
    read_inode(tmp_inode,current_dir,fp);
    int pos=3*(int)MAX_BLOCK_SIZE+64*(int)(current_dir-1);
    //printf("%ld\n",tmp_inode->i_ctime);
    tmp_inode->i_atime=gettime();
    write_inode(tmp_inode,pos,fp);
    ext2_dir_entry entry;
    int read_flag=1;
    int count=0;
    int deal=0;
    int now_dir=0;
    bool block_bitmap[4096];
    bool inode_bitmap[4096];
    fseek(fp,MAX_BLOCK_SIZE,0);
    fread(block_bitmap,sizeof(bool),4096,fp);
    fseek(fp,MAX_BLOCK_SIZE*2,0);
    fread(inode_bitmap,sizeof(bool),4096,fp);

    for(int i=0;i<tmp_inode->i_blocks;i++)
    {
        if(deal==1) break;
        if(tmp_inode->i_block[i]>=512)
        {
             
             fseek(fp,(long)(tmp_inode->i_block[i]+3)*MAX_BLOCK_SIZE,0);
             now_dir=(tmp_inode->i_block[i]+3)*MAX_BLOCK_SIZE;
             int read_flag=1;
             while(read_flag==1)
            {
                 fread(&entry.inode,sizeof(uint16),1,fp);
                 if(entry.inode==(uint16)0) 
                 {
                      printf("ERROR: [%s]No such file or directory\n",dir_name);
                      return 0;
                 }
                 fread(&entry.rec_len,sizeof(uint16),1,fp);
                 fread(&entry.name_len,sizeof(uint8),1,fp);
                 fread(&entry.file_type,sizeof(uint8),1,fp);
                 if(entry.rec_len>0)  //判断不要读入NULL！
                 {
                     fread(entry.name,sizeof(char),entry.rec_len-6,fp);
                 }
                 if(strcmp(entry.name,dir_name)==0)
                 {
                      if(entry.file_type!=2)
                      {
                           printf("ERROR: [%s] is not a directory!\n",dir_name);
                           if(tmp_inode)
                           {
                            free(tmp_inode);
                           }
                           if(new_inode)
                           {
                              free(new_inode);
                           }
                           return 0;
                      }
                      else
                      {
                          deal=1;
                          break;
                      }
                 }
                 count+=entry.rec_len;
                 //printf("%d\n",entry_list[count].inode);
                
            }
        }
        else{
            continue;
        }
    }
    
    uint16 dir_2_rm=entry.inode;
    
    //找到要删除的目录
    char b[1]={0};
    
    fseek(fp,(long)now_dir+count+4,0);//inode号,rec_len保存
    fwrite(b,sizeof(char),entry.rec_len-4,fp);
   
    
    read_inode(new_inode,dir_2_rm,fp);
    for(int i=0;i<new_inode->i_blocks;i++)
    {
        if(deal==1) break;
        if(new_inode->i_block[i]>=512)
        {
             char c[1]={0};
             fseek(fp,(long)(new_inode->i_block[i]+3)*MAX_BLOCK_SIZE,0);
             fwrite(c,sizeof(char),512,fp);
             //update block bitmap;
             block_bitmap[new_inode->i_block[i]-512]=(bool)0;
        }
    }
    
    del_inode(dir_2_rm,fp);

    //update inode bitmap;
    inode_bitmap[dir_2_rm-1]=0;

    fseek(fp,MAX_BLOCK_SIZE,0);
    fwrite(block_bitmap,sizeof(bool),4096,fp);
    fseek(fp,MAX_BLOCK_SIZE*2,0);
    fwrite(inode_bitmap,sizeof(bool),4096,fp);

    
    //更新组描述符信息
    PARENT->fs->gp.bg_free_blocks_count++;
    PARENT->fs->gp.bg_free_inodes_count++;
    PARENT->fs->gp.bg_used_dirs_count--;
    update_group_desc(PARENT);
    
    if(tmp_inode)
    {
        free(tmp_inode);
    }
    if(new_inode)
    {
        free(new_inode);
    }
    rewind(fp);
    fclose(fp);
    return 0;
}

int shell_cmd_create(void)
{
    char dir_name[20];
    rewind(stdin);
    scanf("%s",dir_name);
    //getchar();

    FILE *fp=fopen("FS.bin","rb+");
    //printf("%s\n",dir_name);
    if(!fp) return -1;
    //getchar();
    ext2_inode *tmp_inode=(ext2_inode *)malloc(sizeof(ext2_inode));
    ext2_inode *new_inode=(ext2_inode *)malloc(sizeof(ext2_inode));

    bool block_bitmap[4096];
    bool inode_bitmap[4096];
    fseek(fp,MAX_BLOCK_SIZE,0);
    fread(block_bitmap,sizeof(bool),4096,fp);
    fseek(fp,MAX_BLOCK_SIZE*2,0);
    fread(inode_bitmap,sizeof(bool),4096,fp);

    read_inode(tmp_inode,current_dir,fp);
    int pos=3*(int)MAX_BLOCK_SIZE+64*(int)(current_dir-1);
    //printf("%ld\n",tmp_inode->i_ctime);
    tmp_inode->i_atime=gettime();
    write_inode(tmp_inode,pos,fp);
    ext2_dir_entry entry;
    ext2_dir_entry new_entry;
    //新结点
    new_inode->i_mode=1;
    new_inode->i_blocks=1;
    new_inode->i_size=64;
    new_inode->i_atime=0;
    new_inode->i_ctime=gettime();
    new_inode->i_mtime=0;
    new_inode->i_dtime=0;
    
    for(int i=0;i<8;i++)
    {
         new_inode->i_block[i]=0;
    }
    memset(new_inode->i_pad,'a',8);
    //TODO:分配数据块

    memset(new_entry.name,0,256);

    //int count=0;
    
    int count=0;
    // TODO:i_block[i]<512
    for(int i=0;i<tmp_inode->i_blocks;i++)
    {
        if(tmp_inode->i_block[i]>=512)
        {
             
             fseek(fp,(long)(tmp_inode->i_block[i]+3)*MAX_BLOCK_SIZE,0);
             int read_flag=1;
             while(read_flag==1)
            {
                 fread(&entry.inode,sizeof(uint16),1,fp);
                 if(entry.inode==(uint16)0) break;
                 fread(&entry.rec_len,sizeof(uint16),1,fp);
                 fread(&entry.name_len,sizeof(uint8),1,fp);
                 fread(&entry.file_type,sizeof(uint8),1,fp);
                 if(entry.rec_len>0)  //判断不要读入NULL！
                 {
                     fread(entry.name,sizeof(char),entry.rec_len-6,fp);
                 }
                
                 count+=entry.rec_len;
                 //printf("%d\n",entry_list[count].inode);
                
            }
        }
        else{
            continue;
        }
    }

    int alloc_block=alloc_new_block(new_inode,last_alloc_block);
    //printf("%d\n",alloc_block);
    last_alloc_block=alloc_block;
    block_bitmap[alloc_block]=(bool)1;
    
    //printf("%hd\n",last_alloc_inode);
    int alloc_pos=alloc_new_inode(new_inode,last_alloc_inode);
    //printf("%hd\n",alloc_pos);
    inode_bitmap[alloc_pos-1]=(bool)1;

    int new_pos=3*(int)MAX_BLOCK_SIZE+64*(int)(alloc_pos-1);
    FILE *fp2=fopen("FS.bin","rb+");
    write_inode(new_inode,new_pos,fp2);
    fclose(fp2);
    
   //写索引结点
   
    //写目录项
    if(entry.inode==0)
    {
        fseek(fp,-2L,1);
    }
    else if(entry.inode!=(uint16)0 &&entry.rec_len==0)
    {
        fseek(fp,-4L,1);
    }
    new_entry.inode=alloc_pos;
    new_entry.rec_len=6+strlen(dir_name)+1;
    new_entry.name_len=strlen(dir_name);
    new_entry.file_type=1;  //普通文件
    strcpy(new_entry.name,dir_name);
   

    fwrite(&new_entry.inode,sizeof(uint16),1,fp);
    fwrite(&new_entry.rec_len,sizeof(uint16),1,fp);
    fwrite(&new_entry.name_len,sizeof(uint8),1,fp);
    fwrite(&new_entry.file_type,sizeof(uint8),1,fp);
    fwrite(new_entry.name,sizeof(char),strlen(dir_name)+1,fp);
    
    last_alloc_inode=(uint16)alloc_pos;//这里再更改否则写目录会出错
    
    //更新组描述符信息
    PARENT->fs->gp.bg_free_blocks_count--;
    PARENT->fs->gp.bg_free_inodes_count--;
    PARENT->entry_length++;
    update_group_desc(PARENT);

    //更新位图
    fseek(fp,MAX_BLOCK_SIZE,0);
    fwrite(block_bitmap,sizeof(bool),4096,fp);
    fseek(fp,MAX_BLOCK_SIZE*2,0);
    fwrite(inode_bitmap,sizeof(bool),4096,fp);
    if(tmp_inode)
    {
        free(tmp_inode);
    }
    if(new_inode)
    {
        free(new_inode);
    }
    
    rewind(fp);
    fclose(fp);
    return 0;
}

int shell_cmd_delete(void)
{
    char dir_name[20];
    rewind(stdin);
    scanf("%s",dir_name);
    //getchar();
    
    FILE *fp=fopen("FS.bin","rb+");
    //printf("%s\n",dir_name);
    if(!fp) return -1;
    
    ext2_inode *tmp_inode=(ext2_inode *)malloc(sizeof(ext2_inode));
    ext2_inode *new_inode=(ext2_inode *)malloc(sizeof(ext2_inode));
    read_inode(tmp_inode,current_dir,fp);
    int pos=3*(int)MAX_BLOCK_SIZE+64*(int)(current_dir-1);
    //printf("%ld\n",tmp_inode->i_ctime);
    tmp_inode->i_atime=gettime();
    write_inode(tmp_inode,pos,fp);
    ext2_dir_entry entry;
    int read_flag=1;
    int count=0;
    int deal=0;
    int now_dir=0;
    bool block_bitmap[4096];
    bool inode_bitmap[4096];
    fseek(fp,MAX_BLOCK_SIZE,0);
    fread(block_bitmap,sizeof(bool),4096,fp);
    fseek(fp,MAX_BLOCK_SIZE*2,0);
    fread(inode_bitmap,sizeof(bool),4096,fp);

    for(int i=0;i<tmp_inode->i_blocks;i++)
    {
        if(deal==1) break;
        if(tmp_inode->i_block[i]>=512)
        {
             
             fseek(fp,(long)(tmp_inode->i_block[i]+3)*MAX_BLOCK_SIZE,0);
             now_dir=(tmp_inode->i_block[i]+3)*MAX_BLOCK_SIZE;
             int read_flag=1;
             while(read_flag==1)
            {
                 fread(&entry.inode,sizeof(uint16),1,fp);
                 if(entry.inode==(uint16)0) 
                 {
                      printf("ERROR: [%s]No such file or directory\n",dir_name);
                      return 0;
                 }
                 fread(&entry.rec_len,sizeof(uint16),1,fp);
                 fread(&entry.name_len,sizeof(uint8),1,fp);
                 fread(&entry.file_type,sizeof(uint8),1,fp);
                 if(entry.rec_len>0)  //判断不要读入NULL！
                 {
                     fread(entry.name,sizeof(char),entry.rec_len-6,fp);
                 }
                 if(strcmp(entry.name,dir_name)==0)
                 {
                      if(entry.file_type!=1)
                      {
                           printf("ERROR: [%s] is not a file!\n",dir_name);
                           if(tmp_inode)
                           {
                            free(tmp_inode);
                           }
                           if(new_inode)
                           {
                              free(new_inode);
                           }
                           return 0;
                      }
                      else
                      {
                          deal=1;
                          break;
                      }
                 }
                 count+=entry.rec_len;
                 //printf("%d\n",entry_list[count].inode);
                
            }
        }
        else{
            continue;
        }
    }
    
    uint16 dir_2_rm=entry.inode;
    
    //找到要删除的目录
    char b[1]={0};
    
    fseek(fp,(long)now_dir+count+4,0);//inode号保存
    fwrite(b,sizeof(char),entry.rec_len-4,fp);
    
    read_inode(new_inode,dir_2_rm,fp);
    for(int i=0;i<new_inode->i_blocks;i++)
    {
        if(deal==1) break;
        if(new_inode->i_block[i]>=512)
        {
             char c[1]={0};
             fseek(fp,(long)(new_inode->i_block[i]+3)*MAX_BLOCK_SIZE,0);
             fwrite(c,sizeof(char),512,fp);
             //update block bitmap;
             block_bitmap[new_inode->i_block[i]-512]=(bool)0;
        }
    }
    
    del_inode(dir_2_rm,fp);

    //update inode bitmap;
    inode_bitmap[dir_2_rm-1]=0;

    fseek(fp,MAX_BLOCK_SIZE,0);
    fwrite(block_bitmap,sizeof(bool),4096,fp);
    fseek(fp,MAX_BLOCK_SIZE*2,0);
    fwrite(inode_bitmap,sizeof(bool),4096,fp);

    
    //更新组描述符信息
    PARENT->fs->gp.bg_free_blocks_count++;
    PARENT->fs->gp.bg_free_inodes_count++;
    update_group_desc(PARENT);
    
    if(tmp_inode)
    {
        free(tmp_inode);
    }
    if(new_inode)
    {
        free(new_inode);
    }
    rewind(fp);
    fclose(fp);
    return 0;
}

int shell_cmd_open(void)
{  
    return 0;
}

int shell_cmd_close(void)
{
    return 0;
}

