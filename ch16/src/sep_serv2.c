#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024
void errorHandling(const char *message);

int main(int argc, char *argv[])
{
    int str_len;
    int serv_sock, clnt_sock;
    char buf[BUF_SIZE] = {
        0,
    };

    FILE *readfp;
    FILE *writefp;

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
    clnt_adr_sz = sizeof(clnt_adr);

    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

    readfp = fdopen(clnt_sock, "r");
    writefp = fdopen(dup(clnt_sock), "w"); // 通过dup函数，复制文件描述符，并将其转换为FILE指针

    fputs("From server: Hi,client? \n", writefp);
    fputs("I love all of the world \n", writefp);
    fputs("You are awesome! \n", writefp);
    fflush(writefp);

    shutdown(fileno(writefp), SHUT_WR); // 调用shutdown函数时， 无论复制出多少文件描述都进入半关闭状态
    fclose(writefp);                    // 关闭写端，此时套接字并没有关闭，还有读模式FILE指针

    fgets(buf, sizeof(buf), readfp);
    fputs(buf, stdout);
    fclose(readfp); // 关闭读模式FILE指针，此时才完全关闭套接字

    close(serv_sock);
    return 0;
}

void errorHandling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}