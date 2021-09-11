#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void errorHandling(const char* message);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port> \n", argv[1]);
        exit(1);
    }

    //调用socket函数创建套接字
    int sockServ = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sockServAddr;
    memset(&sockServAddr, 0, sizeof(sockServAddr));
    sockServAddr.sin_family = AF_INET;
    sockServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockServAddr.sin_port = htons(atoi(argv[1]));

    //调用bind函数分配IP地址和端口号
    if (bind(sockServ, (struct sockaddr *)&sockServAddr, sizeof(sockServAddr)) == -1)
    {
        errorHandling("bind() error!");
    }

    //调用listen函数将套接字转化为可接收连接状态
    if (listen(sockServ, 5) == -1)
    {
        errorHandling("listen() error!");
    }

    struct sockaddr_in sockClientAddr;
    socklen_t clientAddrLen = sizeof(sockClientAddr);

    
    //调用accept函数受理连接请求，如果在没有连接请求的情况下调用该函数，则不会返回，直到有连接请求为止。
    int sockClient = accept(sockServ, (struct sockaddr *)&sockClientAddr, &clientAddrLen);
    if (sockClient == -1)
    {
        errorHandling("accept() error!");
    }
    else
    {
        puts("New Client connected...");
    }

    char message[] = "Hello World!";
    //write函数用于传输数据，若程序经accept函数运行到本行，说明已经有了连接请求
    write(sockClient, message, sizeof(message));

    close(sockClient);
    close(sockServ);

    return 0;
}

void errorHandling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
