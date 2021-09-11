# 第 15 章 套接字和标准I/O

## 1、标准I/O函数的优点

### 1.1 标准I/O函数的两大优点

* 标准I/O函数具有良好的移植性（Portability）。
* 标准I/O函数可以利用缓冲提高性能。

在进行TCP通信时，若使用标准I/O函数，将产生以下两种缓冲。

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-15%20%E4%B8%8B%E5%8D%888.02.54.png" alt="截屏2021-05-15 下午8.02.54" style="zoom:50%;" />

使用标准I/O函数缓冲的目的主要是为了提高性能。缓冲并非在所有情况下都能带来卓越的性能。但需要传输的数据越多，有无缓冲带来性能差异越大。可以通过以下两种角度说明性能的提高。

* 传输的数据量
* 数据向输出缓冲移动的次数。

假设一个TCP数据包头信息占 40 个字节：

* 1 个字节10次 40*10=400字节
* 10字节 1次 40*1=40字节

使用I/O缓冲，能显著减少数据包的个数，提高数据传输效率。



### 1.2 标准I/O函数和系统函数之间的性能对比

现编写程序实现以下功能：

将news.txt中的内容，复制到cry.txt中，为使对比更加明显，news.txt的大小应该在300M以上。

使用系统函数实现如下：

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#define BUF_SIZE 3  // 用最短数组长度构成

int main(int argc, char* argv[]){
    int fd1, fd2;   //保存在fd1和fd2中的是文件描述符
    int len;
    char buf[BUF_SIZE];
    clock_t start,end;
    double duration;   // 计算执行时间

    fd1 = open("news.txt", O_RDONLY);
    fd2 = open("cpy.txt", O_WRONLY | O_CREAT | O_TRUNC);
    start = clock();
    while((len=read(fd1,buf,sizeof(buf)))>0){
        write(fd2,buf,len);
    }
    end=clock();
    duration = (double)(end - start) / CLOCKS_PER_SEC;
    printf("running time: %f seconds\n", duration);

    close(fd1);
    close(fd2);
    return 0;
}
```



编译运行

```shell
$ gcc syscpy.c -o ./bin/syscpy 
$ ./bin/syscpy 
running time: 1.183949 seconds
```

可以看到，使用系统函数执行时间为1.18秒



使用标准I/O函数

stdcpy.c

```c
#include <stdio.h>
#include <time.h>
#define BUF_SIZE 3 //用最短长度构成

int main(int argc, char* argv[]){
    FILE *fp1;
    FILE *fp2;
    time_t start,end;
    double duration;

    char buf[BUF_SIZE];

    fp1 = fopen("news.txt","r");
    fp2 = fopen("cpy.txt", "w");
    
    start = clock();
    while(fgets(buf,BUF_SIZE, fp1)!=NULL)
        fputs(buf,fp2);
    end = clock();

    duration = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Running time: %f 秒\n",duration);

    fclose(fp1);
    fclose(fp2);
    return 0;
}
```

编译运行

```shell
$ gcc stdcpy.c -o ./bin/stdcpy 
$ ./bin/stdcpy 
Running time: 0.040688 秒
```

可以看到，使用标准I/O函数只用了0.04 秒。



### 1.3 标准I/O函数的几个缺点

* 不容易进行双向通信
* 有时可能频繁调用fflush()函数
* 需要以FILE结构体指针的形式返回文件描述符

在使用标准I/O函数时， 每次切换读写状态时应调用fflush函数，这也会影响基于缓冲的性能提高



## 2、使用标准 I/O 函数

### 2.1 利用 fdopen 函数转换为FILE结构体指针

使用以下函数，将文件描述符转换为FILE结构体指针

```c
#include <stdio.h>

FILE *fdopen(int fildes, const char * mode);
// 成功时返回转换的FILE结构体指针，失败时返回NULL

/*
	fildes 需要转换的文件描述符
	mode 将创建的FILE结构体指针的模式信息
*/
```



desto.c

```c
#include <stdio.h>
#include <fcntl.h>

int main(int argc, char* argv[]){
    FILE *fp;
    // S_IRWXU 给文件所属用户赋予读写执行权限
    int fd = open("data.dat", O_WRONLY | O_CREAT | O_TRUNC,S_IRWXU);
    if(fd==-1){
        fputs("file open error",stdout);
        return -1;
    }

    fp=fdopen(fd,"w");
    fputs("Network c programming \n", fp);
    fclose(fp);
    return 0;
}
```



编译运行

```shell
$ gcc desto.c -o ./bin/desto 
$ ./bin/desto 
$ cat data.dat 
Network c programming 
```



### 2.2 利用 fileno 函数转换文件描述符

```c
#include <stdio.h>

int fileno(FILE * stream);
// 成功时返回转换后的文件描述符，失败时返回-1
```



todes.c

```c
#include <stdio.h>
#include <fcntl.h>

int main(int argc, char* argv[]){
    FILE *fp;
    int fd = open("data.dat", O_WRONLY|O_CREAT|O_TRUNC);
    if (fd == -1)
    {
        fputs("file open error", stdout);
        return -1;
    }

    printf("First file descriptor: %d \n", fd);
    fp = fdopen(fd,"w");
    fputs("Network c programming \n",fp);
    printf("Second file descriptor: %d \n",fileno(fp));
    fclose(fp);
    return 0;
}
```



编译运行

```shell
$ gcc todes.c -o ./bin/todes 
$ ./bin/todes 
First file descriptor: 3 
Second file descriptor: 3 
```



## 3、基于套接字的标准 I/O 函数使用

使用第4章的echo_server.c和echo_client.c 进行简单修改，只需将代码中的文件描述符，改为FILE结构体指针即可。

echo_stdserv.c

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
    char message[BUF_SIZE];

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
    //write函数用于传输数据，若程序经accept函数运行到本行，说明已经有了连接请求
    //调用accept函数受理连接请求，如果在没有连接请求的情况下调用该函数，则不会返回，直到有连接请求为止。
    for (int i = 0; i < 5; i++)
    {
        //调用accept函数受理连接请求，如果在没有连接请求的情况下调用该函数，则不会返回，直到有连接请求为止。
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
        if (clnt_sock == -1)
            errorHandling("accept() error!");
        else
            printf("connected client %d \n",i+1);
        
        readfp = fdopen(clnt_sock,"r");
        writefp = fdopen(clnt_sock, "w");
        while(!feof(readfp)){
            fgets(message, BUF_SIZE, readfp);
            fputs(message, writefp);
            fflush(writefp);    // 将数据立即传输给客户端
        }
        fclose(readfp);
        fclose(writefp);
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
        fputs("Input message(Q to quit):", stdout);
        fgets(message, BUF_SIZE, stdin);
        if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
            break;
        
        fputs(message,writefp);
        fflush(writefp);
        fgets(message, BUF_SIZE, readfp);
        // 使用标准I/O函数，可以按字符串单位进行数据交换，因此不用在数据的尾部插入0
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
$ gcc echo_stdserv.c -o ./bin/echo_stdserv
$ ./bin/echo_stdserv 9190
connected client 1 
```



```shell
$ gcc echo_client.c -o ./bin/echo_client
$ ./bin/echo_client 127.0.0.1 9190
Connected......
Input message(Q to quit):hello
Message from server: hello
Input message(Q to quit):world
Message from server: world
Input message(Q to quit):123
Message from server: 123
Input message(Q to quit):abc
Message from server: abc
Input message(Q to quit):
```



## 4、习题（参考答案）

（1）请说明标准I/O函数的2个优点。它为何拥有这2个优点？

> 答：标准I/O函数具有良好的移植性（Portability）。标准I/O函数可以利用缓冲提高性能。
>
> 因为标准I/O是按照ANSI C标准定义的，支持大部分操作系统。
>
> 在传输的数据量很大时，利用I/O缓冲可以减少数据交换次数，提高数据传输效率。

（2）利用标准I/O函数传输数据时，下面的想法是错误的：

“调用fputs函数传输数据时，调用后应立即开始发送！”

为何说上述说法是错误的？为了达到这种效果应添加哪些处理过程？

> 答：fputs无法保证立即传输数据，使用fflush()函数来立即发送数据

