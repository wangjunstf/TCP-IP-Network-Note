# 第 16 章 关于I/O流分离的其他内容

## 1、分离I/O流

分离“流”的好处

第10章

* 通过分开输入过程和输出过程降低实现难度
* 与输入无关的操作可以提高速度

第15章

* 为了将FILE指针按读模式和写模式加以区分
* 通过区分读写模式降低实现难度
* 通过区分I/O缓冲提高性能

“流”分离的方法，情况（目的）不同时，带来的好处也有所不同



## 2、“流”分离带来的EOF问题

第15章通过区分FILE指针的读写模式来分离“流”。这样的实现方式如果单纯地调用fclose函数来传递EOF会产生问题，下面用程序验证：

sep_serv.c

```c
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
    char buf[BUF_SIZE]={0,};

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
    writefp = fdopen(clnt_sock, "w");

    fputs("From server: Hi,client? \n",writefp);
    fputs("I love all of the world \n",writefp);
    fputs("You are awesome! \n",writefp);
    fflush(writefp);

    fclose(writefp);
    fgets(buf, sizeof(buf), readfp);
    fputs(buf,stdout);
    fclose(readfp);

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



sep_clnt.c

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
    char buf[BUF_SIZE] = {0,};
    int str_len;
    struct sockaddr_in serv_adr;

    FILE *readfp;
    FILE *writefp;

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

    readfp = fdopen(sock, "r");
    writefp = fdopen(sock, "w");
    while (1)
    {
        if(fgets(buf,sizeof(buf),readfp)==NULL)
            break;
        fputs(buf,stdout);
        fflush(stdout);
    }

    fputs("From CLIENT: Thank you! \n",writefp);
    fflush(writefp);
    fclose(writefp);
    fclose(readfp);
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
$ gcc sep_serv.c -o ./bin/sep_serv
$ ./bin/sep_serv 9190
```



```shell
$ gcc sep_clnt.c -o ./bin/sep_clnt
$ ./bin/sep_clnt 127.0.0.1 9190
Connected......
From server: Hi,client? 
I love all of the world 
You are awesome! 
```



可与看到服务器端关闭输出模式FILE指针后，并没有再收到客户端发送的最后一条信息。服务器端执行fclose(writefp)后，已经彻底关闭套接字，当然无法再收到信息。



## 3、文件描述符的复制和半关闭

### 3.1 终止流时无法半关闭的原因

sep_serv.c示例中的2个FILE指针，文件描述及套接字之间的关系如下：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-16%20%E4%B8%8B%E5%8D%886.42.54.png" alt="截屏2021-05-16 下午6.42.54" style="zoom: 67%;" />



由上图可知，示例sep_serv.c中读模式FILE指针和写模式FILE指针都是基于同一个文件描述符。因此对任意一个FILE指针调用fclose函数都会关闭文件描述符。

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-16%20%E4%B8%8B%E5%8D%886.44.45.png" alt="截屏2021-05-16 下午6.44.45" style="zoom: 67%;" />



那如何进入可以输入但无法输出的半关闭状态呢？创建FILE指针前现复制文件描述符即可。

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-16%20%E4%B8%8B%E5%8D%886.46.34.png" alt="截屏2021-05-16 下午6.46.34" style="zoom: 67%;" />



这时，针对写模式FILE指针调用fclose函数时，只能销毁与该FILE指针相关的文件描述符，无法销毁套接字。

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-16%20%E4%B8%8B%E5%8D%886.49.23.png" alt="截屏2021-05-16 下午6.49.23" style="zoom: 67%;" />



如上图所示：掉哟过fclose函数后还剩1个文件描述符，因此没有销毁套接字。但此时还没有进入半关闭状态，只是准备好了半关闭环境。



### 3.2 复制文件描述符

在同一个进程内完成文件描述符的复制，如下图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-16%20%E4%B8%8B%E5%8D%886.52.32.png" alt="截屏2021-05-16 下午6.52.32" style="zoom: 67%;" />



图16-5 给出的是同一进程内存在2个文件描述符可以同时访问文件的情况。当然，文件描述符的值不能重复。



dup & dup2

下面给出文件描述符的复制方法，通过下列2个函数之一完成。

```c
#include <unistd.h>

int dup(int fildes);
int dup2(int fildes, int fildes2);

// 成功时返回复制的文件描述符，失败时返回-1

/*
	fildes 需要复制的文件描述符
	fildes 明确指定的文件描述符整数值
*/
```

下面编写程序验证dup和dup2的功能：

```c
#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[]){
    int cfd1,cfd2;
    char str1[] = "Hi ~\n";
    char str2[] = "Nice to meet you! \n";

    cfd1 = dup(1);
    cfd2 = dup2(cfd1, 7);
    printf("fd1=%d, fd2=%d \n",cfd1, cfd2);
    write(cfd1, str1, sizeof(str1));
    write(cfd2, str2, sizeof(str2));

    close(cfd1);
    close(cfd2);

    write(1, str1, sizeof(str1));
    close(1);
    write(1, str2, sizeof(str2));
    return 0;
}
```



编译运行

```shell
$ gcc dup.c -o ./bin/dup
$ ./bin/dup
fd1=3, fd2=7 
Hi ~
Nice to meet you! 
Hi ~
```



### 3.3 复制文件描述符后“流”的分离

下面更改sep_serv.c，使其能正常工作。正常工作是指通过服务器端的半关闭状态接收客户端最后发送的字符串。

sep_serv2.c

```c
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
    writefp = fdopen(dup(clnt_sock), "w");    // 通过dup函数，复制文件描述符，并将其转换为FILE指针

    fputs("From server: Hi,client? \n", writefp);
    fputs("I love all of the world \n", writefp);
    fputs("You are awesome! \n", writefp);
    fflush(writefp);
    
    shutdown(fileno(writefp), SHUT_WR);   // 调用shutdown函数时， 无论复制出多少文件描述都进入半关闭状态
    fclose(writefp);                      // 关闭写端，此时套接字并没有关闭，还有读模式FILE指针

    fgets(buf, sizeof(buf), readfp);
    fputs(buf, stdout);
    fclose(readfp);                       // 关闭读模式FILE指针，此时才完全关闭套接字

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



sep_clnt.c不变



编译运行

```shell
$ gcc sep_serv2.c -o ./bin/sep_serv2$ ./bin/sep_serv2 9190From CLIENT: Thank you! 
```



```shell
$ gcc sep_clnt.c -o ./bin/sep_clnt$ ./bin/sep_clnt 127.0.0.1 9190Connected......From server: Hi,client? I love all of the world You are awesome! 
```



无论复制出多少文件描述，均应调用shutdown函数发送EOF并进入半关闭状态。



4、习题（参考答案）

![截屏2021-05-16 下午7.25.53](/Users/wangjun/Library/Application Support/typora-user-images/截屏2021-05-16 下午7.25.53.png)

> （1）
>
> 答：a，文件描述符并不分为输入描述符和输出描述符，一个文件描述符，从一端输入，从另一端输出。
>
> ​		b，复制文件描述符后，将生成新的文件描述符，只是新旧两个文件描述符都指向相同的文件。
>
> ​		c，需要将文件描述符转换为FILE结构体指针才能使用FILE结构体指针进行I/O
>
> ​		e，文件描述符，不区分读和写模式。



> （2）
>
> 答：
>
> a，终止所有文件描述符才能发送EOF
>
> b，此时可能复制了文件描述符，只关闭输出流FILE指针不会发送EOF，需要同时关闭输入流FILE指针或调用shutdown函数才能发送EOF
>
> c，除了关闭文件描述符会发送EOF，调用shutdown函数，无论复制了多少个文件描述符都会发送EOF