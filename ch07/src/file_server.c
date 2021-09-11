#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30
void error_handling(char* message);

int main(int argc, char* argv[]){
    int serv_sd,clnt_sd;
    FILE *fp;
    char buf[BUF_SIZE];
    int read_cnt;

    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz; //记录客户端地址结构体长度
    if(argc != 2){
        printf("Usage: %s <PORT>\n", argv[0]);
        exit(1);    
        //立即终止调用过程，属于该进程的所有文件描述符都被关闭，
        //并且该进程的任何子进程都由进程1(init)继承，并且向该进程的父进程发送SIGCHLD信号。
    }

    fp = fopen("file_server.c","rb");   //以二进制只读方式打开，该文件必须存在
    serv_sd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    bind(serv_sd, (struct sockaddr *)&serv_adr, sizeof(serv_adr));
    listen(serv_sd,5);   //开启监听状态，等待队列数为5

    clnt_adr_sz = sizeof(clnt_adr);
    clnt_sd = accept(serv_sd, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

    while(1){
        read_cnt = fread((void *)buf,1, BUF_SIZE, fp);
        if(read_cnt<BUF_SIZE){
            write(clnt_sd, buf, read_cnt); //每次读取BUF_SIZE，当读取的字节数不足BUF_SIZE，说明已经读取完毕了
            break;
        }
        write(clnt_sd, buf,BUF_SIZE);
    }

    shutdown(clnt_sd,SHUT_WR);
    read_cnt = read(clnt_sd, buf, BUF_SIZE);
    buf[read_cnt] = 0;
    printf("%s\n",buf);
    fclose(fp);
    shutdown(clnt_sd, SHUT_RD);
    close(serv_sd);
    

    return 0;
}

void error_handling(char* message){
    fputs("message",stderr);
    fputc('\n',stderr);
    exit(1);
}