#include<stdio.h>
#include<stdlib.h>
#include<memory.h>
#include<string.h>

#include"fat.h"

#include"disk.h"

int disk_init(BLOCK nums_of_block, unsigned int bytes_per_block)
{
    int64 total_disk_size=nums_of_block*bytes_per_block;
    FILE *fp=fopen("FS.bin","wb+");
    if(fp==NULL)
    {
        printf("disk init error!\n");
        return -1;
    }
    char c[1]={0};
    for(int64 i=0;i<total_disk_size;i++)
    {
        fwrite(c,sizeof(char),1,fp);
    }
    rewind(fp);
    fclose(fp);
    return 0;
}

int disk_uinit()
{
    FILE *fp=fopen("FS.bin","wb");
    if(!fp)
    {
        printf("disk uinit error!\n");
        return -1;
    }
    fclose(fp);
    remove("FS.bin");
    return 0;
}

int disk_read(BLOCK block, void *data)  //磁盘读
{
    FILE *fp=fopen("FS.bin","rb");
    if(!fp)
    {
        return -1;
    }
    if(block<0 || block>TOTAL_DISK_SIZE/512)
    {
        return -1;
    }
    fseek(fp,(long)(block*MAX_BLOCK_SIZE),0);
    fread(data, 1,MAX_BLOCK_SIZE,fp);  //读取一个BLOCK
    rewind(fp); //指针重新到文件首
    fclose(fp);
    return 0;
}

int disk_write(BLOCK block, const void *data) //磁盘写
{
    FILE *fp=fopen("FS.bin","rb+");//这里注意，是加入操作
    if(!fp)
    {
        printf("error!\n");
        return -1;
    }
    if(block<0 || block>TOTAL_DISK_SIZE/512)
    {
        return -1;
    }
    fseek(fp,(long)(block*MAX_BLOCK_SIZE),0);//找到块位置
    fwrite(data,sizeof(char),MAX_BLOCK_SIZE,fp);
    rewind(fp);
    fclose(fp);
    return 0;
}


