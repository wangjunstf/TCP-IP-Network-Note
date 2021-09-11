#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define BUF_SIZE 4
#define EPOLL_SIZE 50

void errorHandling(const char *message);

// 用于测试边缘触发
int main(int argc, char *argv[])
{
    int str_len, state;
    int serv_sock, clnt_sock;
    char message[BUF_SIZE];
    int fds[2];

    int fd_max, fd_num;
    struct epoll_event *ep_events;
    struct epoll_event event;
    int epfd, event_cnt;

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

    epfd = epoll_create(EPOLL_SIZE);
    ep_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

    event.events = EPOLLIN|EPOLLET;
    event.data.fd = serv_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

    while (1)
    {
        event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
        if (event_cnt == -1)
        {
            puts("epoll_wait() error");
            break;
        }
        puts("return epoll_wait ");
        /*
        输出每一次的事件数
        printf("--%d--\n",event_cnt);
        */
        for (int i = 0; i < event_cnt; i++)
        {
            if (ep_events[i].data.fd == serv_sock)
            {
                clnt_adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
                event.events = EPOLLIN;
                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
                printf("Connect client: %d\n", clnt_sock);
            }
            else
            {
                str_len = read(ep_events[i].data.fd, message, BUF_SIZE);
                if (str_len == 0)
                {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
                    close(ep_events[i].data.fd);
                    printf("close client :%d \n", i);
                }
                else
                {
                    write(ep_events[i].data.fd, message, str_len);
                    /*   打印 每次读取的字符串
                    char *s = message;
                    s[str_len]=0;
                    printf("#%s\n",s);
                    */
                }
            }
        }
    }

    close(serv_sock);
    close(epfd);
    return 0;
}

void errorHandling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}