# 第 17 章 优于select的epoll

## 1、epoll理解及应用

### 1.1 基于select的 I/O 复用技术慢的原因

从代码中可以分析其中的不合理的设计：

* 调用select函数后常见的针对所有文件描述符循环语句
* 每次调用select函数时都需要向该函数传递监视对象信息

表面上看好像是循环语句拖慢了select的运行效率，更大的障碍是每次传递监视对象信息。应用程序向操作系统传递数据将对程序造成很大的负担，而且无法通过优化代码解决，因此它将成为性能上的致命弱点。



可不可以仅向操作系统传递一次监视对象，监视范围或内容发生变化时只通知发生变化的事项？

各个系统的支持方式不一样。Linux的支持方式为epoll



### 1.2 select 的优点

大部分操作系统都支持select，select适用于以下两种情况：

* 服务器端接入者少
* 程序应具有兼容性



### 1.3 实现epoll时必要的函数和结构体

#### epoll具有以下优点：

* 无需编写以监视状态变化为目的的针对所有文件描述符的循环语句
* 调用对应于select函数的epoll_wait函数时无需每次传递监视对象的信息



epoll服务器端实现中需要的3个函数：

* epoll_create：创建保存epoll文件描述符的空间
* epoll_ctl: 向空间注册并注销文件描述符
* epoll_wait：与select函数类似，等待文件描述符发生变化



为了添加和删除监视对象文件描述符，select方式中需要FD_SET，FD_CLR函数，但在epoll方式中，通过epoll_ctl函数请求操作系统完成。



在epoll方式中，通过如下结构体epoll_event将发生变化的文件描述符单独集中到一起。

```c
struct epoll_event{
  __uint32_t events;
  epoll_data_t data;
}

typedef union epoll_data{
  void *ptr;
  int fd;
  __uint32_t u32;
  __uint64_t u64;
}epoll_data_t;
```



声明足够大的epoll_event结构体数组后，传递给epoll_wait函数时，发生变化的文件描述符信息将被填入该数组。



#### epoll_create

epoll是从Linux 2.5.44版内核开始引入的。

查看系统内核版本命令如下：

```shell
$ cat /proc/sys/kernel/osrelease
```



函数定义

```c
#include <sys/epoll.h>

int epoll_create(int size);
// 成功时返回epoll文件描述符，失败时返回-1

// size epoll实例的大小
```



调用epoll_create创建的文件描述符保存空间称为"epoll例程"。

Linux 2.6.8之后的内核完全忽略传入epoll_create函数的size参数，因为内核会根据情况调整epoll例程的大小。



epoll_create函数创建的资源与套接字相同，也由操作系统管理。该函数返回的文件描述符主要为了区分epoll例程。与其他文件描述符相同，需要终止时调用close函数。



#### epoll_ctl

用于在epoll例程内部注册监视对象文件描述符。

```c
#include <sys/epoll.h>

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
// 成功时返回0，失败时返回-1

/*
	epfd 用于注册监视对象的epoll例程的文件描述符
	op 用于指定监视对象的添加，删除或更改操作
	fd 需要注册的监视对象文件描述符
	event 监视对象的事件类型
*/
```



举例：

epoll_ctl(A，EPOLL_CTL_ADD, B, C);

第2个参数：EPOLL_CTL_ADD表示”添加“

含义：epoll例程A中注册文件描述符B，主要目的是监视参数C中的事件



epoll_ctl(A，EPOLL_CTL_DEL, B, NULL);

第2个参数：EPOLL_CTL_DEL表示”删除“

含义：从epoll例程中删除文件描述符B



epoll_ctl函数的第2个参数可以传递的形参有以下3种：

* EPOLL_CTL_ADD：将文件描述符注册到epoll例程。
* EPOLL_CTL_DEL：从epoll例程中删除文件描述符。
* EPOLL_CTL_MOD：更改注册的文件描述符的关注事件发生情况。



##### 调用示例

```c
struct epoll_event event;
...
event.events=EPOLLIN; // 发生需要读取数据的情况时
event.data.fd=sockfd;
epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
...
```



epoll_event的成员events中可以保存的常量及所指的事件类型：

* EPOLLIN：需要读取数据的情况。
* EPOLLOUT：输出缓冲区为空，可以立即发送数据的情况。
* EPOLLPRI：收到OOB(out-of-date)数据的情况
* EPOLLRDHUP：断开连接或半关闭的情况，这在边缘触发方式下非常有用。
* EPOLLERR：发生错误的情况。
* EPOLLET：以边缘触发的方式得到事件通知。
* EPOLLONESHOT：发生一次事件后，相应文件描述符不再收到事件通知。因此需要向epoll_ctl函数的第二个参数传递EPOLL_CTL_MOD，再次设置事件。

可以通过位或'|'运算同时传递多个上述参数。



#### epoll_wait 

```c
#include <sys/epoll.h>

int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);
// 成功时返回发生事件的文件描述符数，失败时返回-1

/*
	epfd 表示事件发生监视范围的epoll例程的文件描述符。
	events 保存发生事件的文件描述符集合的结构体地址值
	maxevents 第二个参数中可以保存的最大事件数
	timeout 以1/1000秒为单位的等待时间，传递-1时，一直等待直到事件发生。
*/
```

epoll_wait函数第二个参数需要动态分配。

##### 调用示例

```c
int event_cnt;
struct epoll_event *ep_events;
...
ep_events = malloc(sizeof(struct epoll_event)*EPOLL_SIZE);  //EPOLL_SIZE是宏常量
...
event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
...

```



## 2、基于epoll的回声服务器端

echo_epollserv.c

```c
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define BUF_SIZE 1024
#define EPOLL_SIZE 50

void errorHandling(const char *message);

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
    ep_events = malloc(sizeof(struct epoll_event)*EPOLL_SIZE);

    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

    while (1)
    {
        event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
        if(event_cnt==-1){
            puts("epoll_wait() error");
            break;
        }

        for(int i=0; i<event_cnt; i++){
            if (ep_events[i].data.fd == serv_sock)
            {
                clnt_adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
                event.events=EPOLLIN;
                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
                printf("Connect client: %d\n", clnt_sock);
            }else{
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
```



[echo_mpclient.c](https://github.com/wangjunstf/computer-system/blob/main/%E8%AE%A1%E7%AE%97%E6%9C%BA%E7%BD%91%E7%BB%9C%E5%8E%9F%E7%90%86/TCP-IP-%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B/ch12/src/echo_mpclient.c)



编译运行

```shell
$ gcc echo_epollserv.c -o ./bin/echo_epollserv
$ ./bin/echo_epollserv 9190
Connect client: 5
close client :0 
```



```shell
$ gcc echo_mpclient.c -o ./bin/echo_mpclient
$ ./bin/echo_mpclient 127.0.0.1 9190
Connected......
123
Message from server: 123
hello world
Message from server: hello world
how are you?
Message from server: how are you?
^C
```



## 3、条件触发

**条件触发**

例如：服务器端输入缓冲收到50字节的数据，服务器端操作系统将通知该事件（注册到发生变化的文件描述符）但服务器端读取20字节后还剩30字节的情况下，仍会注册事件。

只要输入缓冲中还剩有数据，就将以事件方式再次注册。

**边缘触发**

输入缓冲收到数据时仅注册1次该事件。即使输入缓冲中还留有数据，也不会再进行注册。

下面通过修改echo_epollserv.c，来验证条件触发的事件注册方式。

### 实现条件触发服务器端

echo_EPLTserv.c

```c
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

    event.events = EPOLLIN;
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
        puts("return epoll_wait");
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
```



[echo_mpclient.c](https://github.com/wangjunstf/computer-system/blob/main/%E8%AE%A1%E7%AE%97%E6%9C%BA%E7%BD%91%E7%BB%9C%E5%8E%9F%E7%90%86/TCP-IP-%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B/ch12/src/echo_mpclient.c)



编译运行

```shell
$ ./bin/echo_EPLTserv 9190
return epoll_wait
Connect client: 5
return epoll_wait
return epoll_wait
return epoll_wait
return epoll_wait
return epoll_wait
return epoll_wait
return epoll_wait
```



```shell
$ gcc echo_mpclient.c -o ./bin/echo_mpclient 
$ ./bin/echo_mpclient 127.0.0.1 9190
Connected......
hello world
Message from server: hello world
how are you!
Message from server: how are you!
```

从运行结果可以看出，每当收到客户端数据时，都会注册该事件，并因此多次调用epoll_wait函数。



可以将上述服务器端echo_EPLTserv.c改成边缘触发，可以将event.events = EPOLLIN;改为event.events = EPOLLIN|EPOLLET;

这样，从客户端接收数据时，仅输出1次"return epoll_wait"字符串，这意味着仅注册1次事件。



测试 echo_EPLTserv2.c

```c
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
        puts("return epoll_wait");
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
```



echo_client.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
void errorHandling(const char *message);

int main(int argc, char *argv[])
{
    int sock;
    char message[BUF_SIZE];
    int str_len;
    struct sockaddr_in serv_adr;

    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        errorHandling("socket() error");
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        errorHandling("connect() error!");
    }
    else
    {
        puts("Connected......");
    }

    while (1)
    {
        fputs("Input message(Q to quit):", stdout);
        fgets(message, BUF_SIZE, stdin);
        if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
            break;

        write(sock, message, strlen(message));
        str_len = read(sock, message, BUF_SIZE - 1);
        message[str_len] = 0;
        printf("Message from server: %s", message);
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
```



编译运行

```shell
$ gcc echo_EPLTserv2.c -o ./bin/echo_EPLTserv2
$ ./bin/echo_EPLTserv2 9190
return epoll_wait
Connect client: 5
return epoll_wait
return epoll_wait
return epoll_wait
```



```shell
$ gcc echo_client.c -o ./bin/echo_client
$ ./bin/echo_client 127.0.0.1 9190
Connected......
Input message(Q to quit):hello world
Message from server: hello world
Input message(Q to quit):
```



可能是版本的原因，此处输出了多次"return epoll_wait"字符串，类似于条件触发。



## 4、边缘触发

边缘触发必知两点内容：

* 通过errno变量验证错误原因。
* 为了完成非阻塞(Non-blocking)I/O，更改套接字特性

为了在发生错误时提供额外的信息，Linux声明了如下全局变量 int errno;

为了引入该变量，需引入头文件error.h头文件，因为此头文件中有上述变量的extern声明。

每种函数发生错误时，保存到errno变量的值都不相同，在需要时查阅即可：

“read函数发现输入缓冲中没有数据可读时返回-1，同时在errno中保存EAGAIN常量”



Linux 提供更改或读取文件属性的如下方法



```c
#include <fcntl.h>

int fcntl(int filedes, int cmd, ...);
// 成功时返回cmd参数相关值，失败时返回-1
/*
	filedes 需要被读取或设置属性的文件描述符
	cmd 表示函数调用目的
*/
```



从上述声明中可以看到，fcntl具有可变参数的形式，如果向第二个参数传递F_GETFL，可以获得第一个参数所指的文件描述符属性

如果向第二个参数传递F_SETFL，可以更改文件描述符属性

例如：将文件（套接字）改为非阻塞模式，需要如下两条语句：

```c
int flag = fcntl(fd, F_GETFL, 0);          // 获取之前设置的属性信息
fcntl(fd,F_SETFL, flag|O_NONBLOCK);        // 在原有属性信息不变的情况下，添加非阻塞标志
```

调用read & write函数，无论是否存在数据，都会形成非阻塞文件(套接字)



### 实现边缘触发的服务器端

首先说明为何需要errno确认错误原因

“边缘触发方式中，接收数据时仅注册1次该事件”

因为这种特点，一旦发生输入相关事件，就应该立即读取输入缓冲中的全部数据。因此需要验证输入缓冲是否为空。

“read函数返回-1，变量errno中的值为EAGAIN时，说明没有数据可读”

既然如此，为何还需要将套接字变成非阻塞模式？变缘触发方式下，以阻塞方式工作的read & write函数有可能引起服务器端的长时间停顿。因此，边缘触发方式中一定要采用非阻塞read & write函数。

接下来给出边缘触发方式工作的回声服务器端示例。

echo_EPETserv.c

```c
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#define BUF_SIZE 4
#define EPOLL_SIZE 50

void errorHandling(const char *message);
void setnonblockingmode(int fd) ;
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
    setnonblockingmode(serv_sock);
    event.events = EPOLLIN;
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
        puts("return epoll_wait");
        for (int i = 0; i < event_cnt; i++)
        {
            if (ep_events[i].data.fd == serv_sock)
            {
                clnt_adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
                setnonblockingmode(clnt_sock);
                event.events = EPOLLIN|EPOLLET;
                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
                printf("Connect client: %d\n", clnt_sock);
            }
            else
            {
                while(1){
                    str_len = read(ep_events[i].data.fd, message, BUF_SIZE);
                    if (str_len == 0)
                    {
                        epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
                        close(ep_events[i].data.fd);
                        printf("close client :%d \n", i);
                        break;
                    }else if(str_len<0){
                        if(errno==EAGAIN)
                            break;
                        
                    }else{
                        write(ep_events[i].data.fd, message, str_len);
                    }
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

void setnonblockingmode(int fd){
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag|O_NONBLOCK);
}
```



echo_mpclient.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
void errorHandling(const char *message);
void read_routine(int sock, char *buf);
void write_routine(int sock, char *buf);
int itoc(int num, char *str);

int main(int argc, char *argv[])
{
    int sock;
    char message[BUF_SIZE];
    int str_len;
    struct sockaddr_in serv_adr;

    pid_t pid;

    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        errorHandling("socket() error");
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        errorHandling("connect() error!");
    }
    else
    {
        puts("Connected......");
    }

    pid = fork();
    if (pid == 0)
    {
        write_routine(sock, message);
    }
    else
    {
        read_routine(sock, message);
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

void read_routine(int sock, char *buf)
{
    while (1)
    {
        int str_len = read(sock, buf, BUF_SIZE);
        if (str_len == 0)
            return;

        buf[str_len] = 0;
        printf("Message from server: %s", buf);
    }
}

void write_routine(int sock, char *buf)
{
    while (1)
    {
        fgets(buf, BUF_SIZE, stdin);
        if (!strcmp(buf, "q\n") || !strcmp(buf, "Q\n"))
        {
            shutdown(sock, SHUT_WR);
            return;
        }

        write(sock, buf, strlen(buf));
    }
}

int itoc(int num, char *str)
{
    char tem[1024];
    int id = 0, id2 = 0;
    while (num)
    {
        int t = num % 10;
        tem[id++] = t + '0';
        num /= 10;
    }
    str[id--] = '\0';
    while (id >= 0)
    {
        str[id2++] = tem[id--];
    }
    return 0;
}
```



编译运行

```shell
$ gcc echo_EPETserv.c -o ./bin/echo_EPETserv
$ ./bin/echo_EPETserv 9190
return epoll_wait
Connect client: 5
return epoll_wait
return epoll_wait
```

```
$ gcc echo_mpclient.c -o ./bin/echo_mpclient
$ ./bin/echo_mpclient 127.0.0.1 9190
Connected......
hello world
Message from server: hello world
I love computer programming
Message from server: I love computer programming
```

可以看到，客户端每发送一次数据，服务器端也相应产生几次事件。



## 5、条件触发和边缘触发的优劣

边缘触发可以做到如下这点：

“可以分离接收数据和处理数据的时间点。”



即使输入缓冲收到数据（注册相应事件），服务器端也能决定读取和处理这些数据的时间点，这样就能给服务器端的实现带来巨大的灵活性



条件触发和边缘触发的区别主要应该从服务器端实现模型的角度讨论。



从实现模型的角度看，边缘触发更有可能带来高性能。



## 7、习题（参考答案）

<img src="/Users/wangjun/Library/Application Support/typora-user-images/截屏2021-05-19 上午12.06.56.png" alt="截屏2021-05-19 上午12.06.56"  />

（1）

* 调用select函数后常见的针对所有文件描述符循环语句
* 每次调用select函数时都需要向该函数传递监视对象信息



（2）

因为套接字是由操作系统管理的，所以无论是select还是epoll都需要将监视对象文件描述符信息通过函数传递给操作系统。



（3）

差异是每次调用select函数时都需要向该函数传递监视对象信息，而epoll只需传递一次。

有些函数必须借助于操作系统，select函数与文件描述符有关，更准确地说，是监视套接字变化的函数，而套接字是由操作系统管理的，所以select函数需要借助于操作系统才能完成。



（4）

* 服务器端接入者少
* 程序应具有兼容性



（5）

二者的区别主要在于服务器端实现模型。条件触发方式中，只要输入缓冲中有数据，就会一直通知该事件，而边缘触发，输入缓冲收到数据时仅注册一次该事件，即使输入缓冲中还留有数据，也不会再进行注册。



（6）

原因：输入缓冲收到数据时仅注册一次该事件，即使输入缓冲中还留有数据，也不会再进行注册。

优点：给服务器端的实现带来巨大的灵活性。



(7)

```c++
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <new>
#include <list>

using namespace std;

#define BUF_SIZE 1024
#define EPOLL_SIZE 50

void errorHandling(const char *message);

int main(int argc, char *argv[])
{
    list<int>clnts;
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
    ep_events = new epoll_event[EPOLL_SIZE];
    

    event.events = EPOLLIN;
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
                clnts.push_back(clnt_sock);
            }
            else
            {
                str_len = read(ep_events[i].data.fd, message, BUF_SIZE);
                if (str_len == 0)
                {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
                    close(ep_events[i].data.fd);
                    printf("close client :%d \n", i);
                    clnts.remove(ep_events[i].data.fd);
                }
                else
                {
                    for(auto it = clnts.begin(); it!=clnts.end(); it++){
                        write(*it, message, str_len);
                    }
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
```



chat_epollETserver.cpp

```c++
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <new>
#include <list>

using namespace std;

#define BUF_SIZE 1024
#define EPOLL_SIZE 50

void errorHandling(const char *message);
void setnonblockingmode(int fd);

int main(int argc, char *argv[])
{
    list<int> clnts;
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
    ep_events = new epoll_event[EPOLL_SIZE];

    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);
    setnonblockingmode(serv_sock);
    while (1)
    {
        event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
        if (event_cnt == -1)
        {
            puts("epoll_wait() error");
            break;
        }

        for (int i = 0; i < event_cnt; i++)
        {
            if (ep_events[i].data.fd == serv_sock)
            {
                clnt_adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
                setnonblockingmode(clnt_sock);
                event.events = EPOLLIN|EPOLLET;
                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
                printf("Connect client: %d\n", clnt_sock);
                clnts.push_back(clnt_sock);
            }
            else
            {
                while(1){
                    str_len = read(ep_events[i].data.fd, message, BUF_SIZE);
                    if (str_len == 0)
                    {
                        epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
                        close(ep_events[i].data.fd);
                        printf("close client :%d \n", i);
                        clnts.remove(ep_events[i].data.fd);
                    }
                    else
                    {
                        for (auto it = clnts.begin(); it != clnts.end(); it++)
                        {
                            write(*it, message, str_len);
                        }
                    }
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

void setnonblockingmode(int fd){
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag|O_NONBLOCK);
}
```

