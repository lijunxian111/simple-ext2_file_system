#include <sys/unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "cmd.h"

int exit_my(void)
{
    sleep(0.5);
    return 1;
}

char* match(char *str, const char *want) {
    const char *ptr=want;
    while(*want != '\0') {
        //如果str先到达尾部，也会是NULL
        if(*str++ != *ptr++) {
            return NULL;
        }
    }
    return str;
}
 
int del_substr(char *str, char const *substr) {
    char *next;
    while (*str != '\0' ) {
        next = match(str, substr);
        //找到substr
        if (next != NULL) {
            break;
        }
        str++;
    }
    //如果str指向了尾部，说明没有找到
    if (*str == '\0') {
        return 0;
    }
    //将查找到的子串的尾部拷贝到查找到的子传的首部
    while(*str != '\0'){
        *str++ = *next++;
    }
    return 1;
}


