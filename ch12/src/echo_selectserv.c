#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define BUF_SIZE 1024
void errorHandling(const char *message);


int main(int argc, char *argv[])
{
    int str_len, state;
    int serv_sock, clnt_sock;
    char message[BUF_SIZE];
    int fds[2];

    int fd_max, fd_num;
    fd_set reads,cpy_reads;
    struct timeval timeout;


    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    if (argc != 2)
    {
        printf("Usage: %s <port> \n", argv[1]);
        exit(1);
    }

    //调用socket函数创建套接字
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
    {
        errorHandling("socket() error");
    }
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    //调用bind函数分配IP地址和端口号
    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        errorHandling("bind() error!");
    }

    //调用listen函数将套接字转化为可接收连接状态
    if (listen(serv_sock, 5) == -1)
    {
        errorHandling("listen() error!");
    }
    //write函数用于传输数据，若程序经accept函数运行到本行，说明已经有了连接请求
    //调用accept函数受理连接请求，如果在没有连接请求的情况下调用该函数，则不会返回，直到有连接请求为止。
   
    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);
    fd_max = serv_sock;
    
    while(1){
        cpy_reads = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 5000;
        if((fd_num=select(fd_max+1, &cpy_reads, 0,0,&timeout))==-1){
            break;
        }

        if(fd_num == 0){
            continue;    //没有消息
        }
        for(int i=0; i<fd_max+1; i++){
            if(FD_ISSET(i,&cpy_reads)){
                if(i==serv_sock){
                    clnt_adr_sz = sizeof(clnt_adr);
                    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
                    FD_SET(clnt_sock, &reads);
                    if(fd_max<clnt_sock){
                        fd_max = clnt_sock;
                    }

                    printf("Connect client: %d\n",clnt_sock);
                }else{
                    str_len = read(i, message, BUF_SIZE);
                    if(str_len==0){
                        FD_CLR(i,&reads);
                        close(i);
                        printf("close client :%d \n",i);
                    }else{
                        write(i,message,str_len);
                    }
                }
            }
        }
    }

    close(serv_sock);
    return 0;
}

void errorHandling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}


