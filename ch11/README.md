# 第 11 章 进程间通信

因为不同的进程占用不同的内存空间，无法直接进行通行，只能借助其他特殊方法完成。

**本章介绍进程间通信的一种方式：管道**

## 1、进程间通信的方式

### 1.1 通过管道实现进程间通信

为了完成进程间通信，需要创建管道。管道并非属于进程的资源，而是和套接字一样，属于操作系统。管道就是一段内存空间，通过使用管道，两个进程通过操作系统提供的内存空间进行通信。

管道的创建

```c
#include <unistd.h>

int pipe(int filedes[2]);
//成功时返回0，失败时返回-1
/*
	filedes[0] 通过管道接收数据时使用的文件描述符，即管道出口。
	filedes[1] 通过管道传输数据时使用的文件描述符，即管道入口
*/
```

#### 示例

```c
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>

#define BUF_SIZE 30

int main(int argc, char* argv[]){
    int fds[2];
    char str[] = "Who are you?";
    char buf[BUF_SIZE];
    pid_t pid;

    pipe(fds);
    pid = fork();
    if(pid == 0 ){
        write(fds[1],str, sizeof(str));
    }else{
        read(fds[0],buf,BUF_SIZE);
        puts(buf);
    }
    return 0;
}
```

#### 编译运行

```shell
$ gcc pipe1.c -o ./bin/pipe1
$ ./bin/pipe1
Who are you?
```

上述示例中，子进程往管道写数据，父进程往管道读取数据，如下图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-07%20%E4%B8%8B%E5%8D%888.59.05.png" alt="截屏2021-05-07 下午8.59.05" style="zoom:33%;" />

### 1.2 通过管道进行进程间双向通信

下面创建2个进程通过1个管道进行双向数据交换，其通信方式如下图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-07%20%E4%B8%8B%E5%8D%889.04.34.png" alt="截屏2021-05-07 下午9.04.34" style="zoom: 67%;" />

#### 代码示例

```c
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>

#define BUF_SIZE 30

int main(int argc, char *argv[])
{
    int fds[2];
    char str1[] = "Who are you?";
    char str2[] = "Thank for your message";
    char buf[BUF_SIZE];
    pid_t pid;

    pipe(fds);
    pid = fork();
    if (pid == 0)
    {
        write(fds[1], str1, sizeof(str1));
        sleep(2);
        read(fds[0], buf, BUF_SIZE);
        printf("Child proc output: %s \n",buf);
    }
    else
    {
        read(fds[0], buf, BUF_SIZE);
        printf("Parent proc output: %s \n",buf);
        write(fds[1], str2, sizeof(str2));
        sleep(3);
    }
    return 0;
}
```



#### 编译运行

```shell
$ gcc pipe2.c -o ./bin/pipe2
$ ./bin/pipe2
Parent proc output: Who are you? 
Child proc output: Thank for your message 
```



子进程向管道写入输入后，休眠2秒，父进程从管道读取完数据，并向管道写入数据，然后又进入休眠。子进程休眠结束后从管道读取数据。

这里有个疑问，是子进程先写入，还是父进程先读取？

> 其实这个没有区别，如果是父进程先尝试读取，因为管道没有数据会进入阻塞状态，直到子进程向管道写入数据，子进程向管道写入了数据，就进入了休眠模式，父进程在子进程休眠过程中向管道写入数据，然后进入休眠，子进程在父进程休眠期间从管道读取数据。



从上面的示例中，可以看到只用1个管道进行双向通信并非易事。为了实现这一点，程序需要预测并控制运行流程，这在每种系统中都不同，可以视为不可能完成的任务。



既然如此，该如何完成双向通信？答案是“使用2个管道。”

### 1.3 使用2个管道完成双向通信

使用2个管道完成双向通信，如下图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-07%20%E4%B8%8B%E5%8D%889.17.36.png" alt="截屏2021-05-07 下午9.17.36" style="zoom:50%;" />

#### 代码示例

```c
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>

#define BUF_SIZE 30

int main(int argc, char *argv[])
{
    int fds1[2], fds2[2];
    char str1[] = "Who are you?";
    char str2[] = "Thank for your message";
    char buf[BUF_SIZE];
    pid_t pid;

    pipe(fds1);
    pipe(fds2);
    pid = fork();
    if (pid == 0)
    {
        write(fds1[1], str1, sizeof(str1));
        read(fds2[0], buf, BUF_SIZE);
        printf("Child proc output: %s \n", buf);
    }
    else
    {
        read(fds1[0], buf, BUF_SIZE);
        printf("Parent proc output: %s \n", buf);
        write(fds2[1], str2, sizeof(str2));
        //sleep(3);
    }
    return 0;
}
```



#### 编译运行

```shell
$ gcc pipe3.c -o ./bin/pipe3
$ ./bin/pipe3
Parent proc output: Who are you? 
Child proc output: Thank for your message 
```



## 2、运用进程间通信

接下来扩展第10章echo_mpserv.c，添加以下功能：将回声客户端传输的字符串按序保存到文件中。

我们将该任务委托给另外的进程。换言之，另行创建进程，从向客户端提供服务的进程读取字符串信息。当然，该过程需要创建用于接收数据的管道。

#### 代码示例

echo_storeserv.c

```c
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#define BUF_SIZE 1024
void errorHandling(const char *message);
void read_childproc(int sig);

int main(int argc, char *argv[])
{
    int str_len, state;
    int serv_sock, clnt_sock;
    char message[BUF_SIZE];
    int fds[2];

    pid_t pid;
    struct sigaction act;
    act.sa_handler = read_childproc;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    state = sigaction(SIGCHLD, &act, 0);

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

    pipe(fds);
    pid = fork();
    if(pid==0){
        FILE *fp = fopen("echomsg.txt","wt");
        char msgbuf[BUF_SIZE];
        int len;
        for(int i=0; i<5; i++){
            len = read(fds[0],msgbuf, BUF_SIZE);
            fwrite((void*)msgbuf, 1 , len, fp);
            
        }

        fclose(fp);
        return 0;
    }

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        //调用accept函数受理连接请求，如果在没有连接请求的情况下调用该函数，则不会返回，直到有连接请求为止。
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
        if (clnt_sock == -1)
        {
            continue;
        }
        else
        {
            printf("new connected client\n");
        }
        pid = fork();
        if (pid == -1)
        {
            close(clnt_sock);
            continue;
        }
        if (pid == 0)
        {
            close(serv_sock);
            while ((str_len = read(clnt_sock, message, BUF_SIZE)))
            {
                //str_len表示读取到的字符串长度
                write(clnt_sock, message, str_len);
                write(fds[1],message,str_len);
            }

            close(clnt_sock);
            puts("client disconnected...");
            return 0;
        }
        else
        {
            close(clnt_sock);
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

void read_childproc(int sig)
{
    pid_t pid;
    int status;
    pid = waitpid(-1, &status, WNOHANG); //-1代表可以等待任意子进程终止  WNOHANG即使没有终止的子进程也不会进入阻塞状态，而是返回0并退出函数
    printf("remove proc id: %d\n", pid);
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

#### 编译运行

服务器端

```shell
$ gcc echo_storeserv.c -o ./bin/echo_storeserv 
$ ./bin/echo_storeserv 9191
new connected client
remove proc id: 7616
client disconnected...
```

客户端

```shell
$ gcc echo_mpclient.c -o ./bin/echo_mpclient
$ ./bin/echo_mpclient 127.0.0.1 9191
Connected......
hello world
Message from server: hello world
how are you
Message from server: how are you
123        
Message from server: 123
code
Message from server: code
last message
Message from server: last message
q
```

服务器端接收完5条数据后，可以在echomsg.txt中验证保存的字符串。



## 3、习题(参考答案)

(1) 什么是进程间通信？分别从概念上和内存的角度进行说明？

> 答：进程间通信是指两个不同进程间可以交换数据。
>
> 从内存角度看，操作系统提供了两个进程可以同时访问的内存空间，从而可以让两个进程进行通信。



(2) 进程间通信需要特殊的IPC机制，这是由操作系统提供的。进程间通信 为何需要操作系统的帮助？

> 答：因为不同的进程具有完全独立的内存结构，无法直接进行通信。只能借助一块两个进程都可以访问的内存空间来进行通信。这块共享内存空间由操作系统提供。



(3) "管道"是典型的IPC技法。关于管道，请回答如下问题。

a. 管道是进程间交换数据的路径。如何创建此路径，由谁创建？

> 答：通过调用pipe函数创建，传递一个含有两个元素的数组，0号元素表示接收数据时使用的文件描述符，1号元素表示发送数据时使用的文件描述符。由操作系统创建。

b. 为了完成进程间通信，2个进程需同时连接管道。那2个进程如何连接到同一个管道？

> 答：调用fork函数，子进程会复制父进程的所有资源，包括文件描述符。因而父子进程都具有管道的I/O文件描述符。子进程和父进程都可以向同一个管道发送和接收数据，从而实现进程间通信。

c. 管道允许进行2个进程间的双向通信。双向通信中需要注意哪些内容？

> 注意读取和写入的次序，如果在写之前尝试读，那么调用read函数会进入阻塞状态，直到有数据进入管道。



(4) 编写示例复习IPC技法，使2个进程相互交换3次字符串。当然，这2个进程应该具有父子关系，各位可指定任意字符串。

```c
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#define BUF_SIZE 30

int main(int argc, char* argv[]){
    int fds[2];
    char buf[BUF_SIZE];
    char str1[] = "hello I am client.";
    char str2[] = "Nice to meet you.";
    char str3[] = "Nice to meet you too.";
    pid_t pid;
    int len;

    pipe(fds);
    pid = fork();
    if(pid==0){
        write(fds[1],str1,strlen(str1));
        sleep(2);
        len = read(fds[0], buf, BUF_SIZE);
        buf[len] = 0;
        printf("Child proc output: %s \n", buf);
        write(fds[1], str3, strlen(str3));
    }else{
        len = read(fds[0],buf,BUF_SIZE);
        buf[len] = 0;
        printf("Parent proc output: %s \n",buf);
        write(fds[1],str2,strlen(str2));
        sleep(3);
        len = read(fds[0], buf, BUF_SIZE);
        buf[len] = 0;
        printf("Parent proc output: %s \n", buf);
    }
    return 0;
}
```



**子进程**向管道**写入数据**，然后**睡眠2秒**，在这2秒内**父进程**从管道**读取数据**，并向管道**写入数据**，**父进程睡眠3秒**

**子进程**在父进程睡眠3秒的时间中读取父进程写入的数据，并写入数据。

父进程读取子进程写入的数据。



