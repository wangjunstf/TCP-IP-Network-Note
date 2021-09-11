# 第 12 章 I/O复用

## 1、多进程服务器端的缺点和解决方法

为了构建并发服务器端，只要有客户端连接请求就会创建新进程。这的确是一种解决方法，但并非十全十美，因为创建进程需要付出极大代价，需要大量的运算和内存空间，由于每个进程都具有独立的内存空间，所以相互间的数据交换也要求采用相对复杂的方法（IPC属于相对复杂的通信方法）。



那有什么办法在不创建进程的同时向多个客户提供服务？答案是**I/O复用**。



## 2、理解I/O复用

“复用”在通信工程领域的含义是：”在1个通信频道中传递多个数据（信号）的技术“

更通俗一些，“复用”的含义是“为了提高物理设备的效率，用最少的物理要素传递最多数据时使用的技术”



服务器端引入复用技术，可以减少所需进程数。



I/O复用可以理解为用一个进程可以为多个客户提供服务。



## 3、理解select函数并实现服务器端

运用select函数是最具代表性的实现复用服务器端方法。

### 3.1 select函数的功能和调用顺序

使用select函数时可以将多个文件描述符集中到一起统一监视，监视的项目如下：

* 是否存在套接字接收数据？
* 无需阻塞传输数据的套接字有那些？
* 哪些套接字发生了异常？

select函数是整个I/O复用的全部内容，以下为select函数的调用顺序

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-08%20%E4%B8%8B%E5%8D%889.35.55.png" alt="截屏2021-05-08 下午9.35.55" style="zoom: 67%;" />

### 3.2 设置文件描述符

利用select函数可以同时监视多个文件描述符。当然，监视文件描述符可以视为监视套接字。集中时也要按照监视项（接收，传输，异常）进行区分，共分成3类。

每一类都用fd_set结构体表示，如下图所示：

![截屏2021-05-08 下午11.02.40](https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-08%20%E4%B8%8B%E5%8D%8811.02.40.png)

上图是存有0和1的位数组，下标表示文件描述符，数组元素为1表示该文件描述符为监视对象。

要设置某个文件描述为监视对象（称为注册），或者取消对某个文件描述符的监视，需要更改每一位的值。在fd_set变量中注册或更改值的操作都由下列宏完成。

* FD_ZERO(fd_set *fdset)    : 将fd_set变量的所有位初始化为0
* FD_SET(int fd, fd_set *fdset)  : 在参数fd_set指向的变量中注册文件描述符fd的信息
* FD_CLR(int fd, fd_set *fdset)  : 从参数fdset指向的变量中清除文件描述符fd的信息
* FD_ISSET(int fd, fd_set *fdset) : 若参数fdset指向的变量中包含文件描述符fd的信息，则返回真



<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-08%20%E4%B8%8B%E5%8D%8811.12.06.png" alt="截屏2021-05-08 下午11.12.06" style="zoom: 67%;" />

 

### 3.3 设置监视范围及超时

```c
#include <sys/select.h>
#include <sys/time.h>

int select(int maxfd, fd_set *readset, fd_set *writeset, fd_set *exceptset, const struct timeval *timeout);
//成功时返回 发生事件的文件描述符数 失败时返回-1

/*
	maxfd 监视对象文件描述符数量
	readset 将所有关注"是否存在待读数据"的文件描述符注册到fd_set型变量，并传递其地址值
	writeset 将所有关注"是否可传输无阻塞数据"的文件描述符注册到fd_set型变量，并传递其地址值
	exceptset 将所有关注“是否发生异常”的文件描述符注册到fd_set型变量，并传递其地址值
	timeout 调用select函数后为防止陷入无限阻塞状态，传递超时(time-out)信息
*/
```



select函数只有在监视的文件描述符发生变化时才返回，如果未发生变化，就会进入阻塞状态。指定超时时间就是为了防止这种情况的发生。struct timeval的结构体内容

```c
struct timeval{
	long tv_sec;   //秒
	long tv_usec;  //毫秒
}
```

如果想设置超时，则传递NULL



### 3.4 调用select函数后查看结果

调用select函数后，可以知道哪些文件描述符发生了变化，发生变化的文件描述符被标记为1，没有发生变化的文件描述符被标记为0，如下图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-08%20%E4%B8%8B%E5%8D%8811.27.36.png" alt="截屏2021-05-08 下午11.27.36" style="zoom: 67%;" />



每次调用select函数后，fd_set值会被修改，因此在调用select函数时，传递一个值与fd_set变量相等的临时变量。



### 3.5 select函数调用示例

#### 代码

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>

#define BUF_SIZE 30

int main(int argc, char* argv[]){
    fd_set reads,temps;
    int result, str_len;
    char buf[BUF_SIZE];
    struct timeval timeout;

    FD_ZERO(&reads);
    FD_SET(0,&reads);

    while(1){
        temps = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        result = select(1, &temps, 0, 0, &timeout);
        if(result==-1){
            puts("select() error!");
            break;
        }else if(result==0){
            puts("Time-out!");
        }else{
            if(FD_ISSET(0,&temps)){
                str_len = read(0,buf,BUF_SIZE);
                buf[str_len] = 0;
                printf("message from console: %s",buf);
            }
        }
    }
    return 0;
}
```

#### 编译运行

```shell
$ gcc select.c ./bin/select 
$ ./bin/select 
hello
message from console: hello
123
message from console: 123
Time-out!
Time-out!
```



## 4、实现I/O复用服务器端

#### 代码

echo_selectserv.c

```c
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

```

> 当客户端请求与服务器端建立连接时，本质上也是发送数据，因此对于服务器来说，建立连接也按接收数据来算。

客户端可以选择前面章节的任意一个客户端，这里选择第11章的

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

#### 编译运行

服务器端

```shell
$ gcc echo_selectserv.c -o ./bin/echo_selectserv 
$ ./bin/echo_selectserv 9190
Connect client: 4
close client :4 
```

客户端

```shell
$ gcc echo_mpclient.c -o ./bin/echo_mpclient 
$ ./bin/echo_mpclient 127.0.0.1 9190
Connected......
hello world
Message from server: hello world
how are you
Message from server: how are you
q
```



## 5、习题（参考答案）

（1）请解释复用技术的通用含义，并说明何为I/O复用。

> 答：“复用”的通用含义是“为了提高物理设备的效率，用最少的物理要素传递最多数据时使用的技术”
>
> I/O复用可以理解为用一个进程为多个客户提供服务。

（2）多进程并发服务器的缺点有哪些？如何在I/O复用服务端中弥补？

> 答：创建进程需要付出极大代价，需要大量的运算和内存空间，进程间通信也比较复杂。
>
> 可以使用select函数，用一个进程为多个客户端提供服务。

（3）复用服务器端需要select函数。下列关于select函数使用方法的描述错误的是？

> 答：c
>
> a正确，调用select函数前需要将集中I/O监视对象的文件描述符
>
> b正确。d正确

（4）select函数的观察对象中应包含服务器端套接字（监听套接字），那么应将其包含到哪一类监听对象集合？请说明原因？

> 答：readset集合，当客户端请求与服务器端建立连接时，本质上也是发送数据，因此对于服务器来说，建立连接即接收数据。

（5）略

