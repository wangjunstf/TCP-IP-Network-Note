#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void errorHandling(const char *message);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <ip> <port> \n", argv[0]);
        exit(1);
    }

    //创建套接字，此时套接字并不马上分为服务器端和客户端。
    //如果紧接着调用bind,listen函数，将成为服务器端套接字；
    //如果调用connect函数，将成为客户端套接字
    int sock = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = inet_addr(argv[1]);
    sockAddr.sin_port = htons(atoi(argv[2]));

    //调用connect函数，向服务器端发送连接请求
    if (connect(sock, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) == -1)
    {
        errorHandling("connect() error!");
    }

    char buf[32];
    int readLen = read(sock, buf, sizeof(buf) - 1);
    if (readLen > 0)
    {
        buf[readLen] = 0;
        printf("Message from server: %s \n", buf);
    }

    close(sock);

    return 0;
}

void errorHandling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
