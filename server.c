/**
 * Copyright (c) 2020-2021 aplyc1a <aplyc1a@protonmail.com>
 * 
 * How to start:
 * step 1: gcc server.c -lpthread -o server
 * step 2: ./server -p 123456 -f /usr/bin/ls -v
 * 
 **/

#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

///////////////////////////////////////////////
//          CONFIGURATION   HERE             //
///////////////////////////////////////////////
#define SOCK_PORT 40050
#define BUFFER_LENGTH 1024
#define MAX_CONN_LIMIT 512
#define CHUNK_SIZE 50
#define USLEEP_TIME 100 //microseconds
uint8_t verbose=0;
uint8_t auth_code[BUFFER_LENGTH] = {0};
uint8_t filename[BUFFER_LENGTH] = {0};
///////////////////////////////////////////////
//          CONFIGURATION   DONE             //
///////////////////////////////////////////////

void get_random_work_id(uint8_t *arr_space, int length)
{
    uint8_t uint8_tset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    srand(time(NULL));
    for (int i = 0; i < length - 1; i++){
        arr_space[i] = uint8_tset[rand() % 52]; 
    }
    arr_space[length - 1] = '\0';
}


static void data_handle(void * sock_fd)
{
    int32_t fd = *((int *)sock_fd);
    uint8_t ret;
    uint8_t data_recv[BUFFER_LENGTH];
    uint8_t work_id[8] = {0};
    
    if (verbose) 
    {
        get_random_work_id(work_id,sizeof(work_id)-1);
        printf("[+] Start doing task-%s \n",work_id);
    }
    memset(data_recv,0,BUFFER_LENGTH);
    ret = read(fd,data_recv,BUFFER_LENGTH);
    if(ret == 0)
    {
        fprintf(stderr,"client has closed!\n");
        goto end;
    }
    if(ret == -1)
    {
        fprintf(stderr,"read error!\n");
        goto end;
    }
        
    if(strcmp(data_recv, auth_code)!=0)
    {
        fprintf(stderr,"auth failed!\n");
        if(verbose){printf("AUTH: %s\n",data_recv);}
        write(fd,"EF",strlen("EF"));
        goto end;
    }
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL){
        fprintf(stderr,"open core-file error!\n");
        goto end;
    }
    
    long long len = 0;
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    rewind(fp);
    if (verbose) printf("[!] The `%s <%ld Bytes>` is being downloaded.\n", filename, len);
    
    uint8_t stage1_data[20];
    sprintf(stage1_data, "V %ld", len);
    
    if(write(fd,stage1_data,strlen(stage1_data)) == -1)
    {
        fprintf(stderr,"socket-write error!\n");
        goto end;
    }
    
    uint8_t buff[CHUNK_SIZE];
    int32_t i = 0;
    while (1)
    {
        memset(buff, 0, sizeof(buff));
        if (i + sizeof(buff) > len) {
            ret = fread(buff, 1, i + sizeof(buff) - len, fp);
            write(fd, buff, i + sizeof(buff) - len);
            if (verbose) {for(int j=0; j<i + sizeof(buff) - len; j++){printf("%02x ",buff[j]);}; printf("\n");}
            break;
        } else {
            ret = fread(buff, 1, sizeof(buff), fp);
            write(fd, buff, sizeof(buff));
            if (verbose) {for(int j=0; j<sizeof(buff); j++){printf("%02x ",buff[j]);}; printf("\n");}
            i = i + sizeof(buff);
        }
        usleep(USLEEP_TIME);
    }
    
end:
    close(fd); 
    //if(verbose) { printf("[-] Stop doing task-%s \n",work_id); }
    pthread_exit(NULL);
}

void start_server(){
    int sockfd_server;
    int sockfd;
    int fd_temp;
    struct sockaddr_in s_addr_in;
    struct sockaddr_in s_addr_client;
    int client_length;

    sockfd_server = socket(AF_INET,SOCK_STREAM,0);
    assert(sockfd_server != -1);

    memset(&s_addr_in,0,sizeof(s_addr_in));
    s_addr_in.sin_family = AF_INET;
    s_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    s_addr_in.sin_port = htons(SOCK_PORT);
    fd_temp = bind(sockfd_server,(struct sockaddr *)(&s_addr_in),sizeof(s_addr_in));
    if(fd_temp == -1)
    {
        fprintf(stderr,"bind error!\n");
        exit(1);
    }

    fd_temp = listen(sockfd_server,MAX_CONN_LIMIT);
    if(fd_temp == -1)
    {
        fprintf(stderr,"listen error!\n");
        exit(1);
    }
    
    printf("[*] Listening on %d ...\n", SOCK_PORT);
    while(1)
    {
        pthread_t thread_id;
        client_length = sizeof(s_addr_client);

        sockfd = accept(sockfd_server,(struct sockaddr *)(&s_addr_client),(socklen_t *)(&client_length));
        if(sockfd == -1)
        {
            fprintf(stderr,"Accept error!\n");
            continue;
        }
        printf("[+] %s:%d is knocking... \n", inet_ntoa(s_addr_client.sin_addr), s_addr_client.sin_port);
        if(pthread_create(&thread_id, NULL, (void *)(&data_handle), (void *)(&sockfd)) == -1)
        {
            fprintf(stderr,"pthread_create error!\n");
            break;
        }
    }

    int ret = shutdown(sockfd_server,SHUT_WR);
    assert(ret != -1);

}

void show_usage(){
    printf("console > show usage\n");
    printf("--------------------------------------\n");
    printf("    -p : set password\n");
    printf("    -f : set the elf file to be download\n");
    printf("    -h : show usage\n");
    printf("    -v : run server in verbose mode\n");
    printf("--------------------------------------\n");
    printf("eg: ./server -p 123456 -f xmr\n");
    printf("--------------------------------------\n");
}

int main(int argc, char *argv[])
{
    int32_t opt=0;
    while((opt=getopt(argc,argv,"hvf:p:"))!=-1)
    {
        switch(opt)
        {
            case 'h':
                show_usage();
                exit(1);
            case 'p':
                strcpy(auth_code, optarg);
                break;
            case 'f':
                strcpy(filename, optarg);
                break;
            case 'v':
                verbose=1;
                break;
            default:
                show_usage();
                exit(1);
        }
    }
    
    if(strlen(filename)>BUFFER_LENGTH )
    {
        fprintf(stderr,"buffer overflows!\n"); 
        exit(1);
    }
    
    if(strlen(auth_code)>BUFFER_LENGTH )
    {
        fprintf(stderr,"buffer overflows!\n"); 
        exit(1);
    }
    
    if(strlen(auth_code) && strlen(filename))
    {
        printf("console > start server\n");
        start_server();
    }
    else
    {
        show_usage();
        exit(1);
    }
    printf("console > exit\n");
    return 0;
}