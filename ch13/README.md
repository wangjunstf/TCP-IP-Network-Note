# 第 13 章 多种I/O函数

之前的章节中，在Linux环境下使用read & write函数完成数据I/O。本章将介绍send & recv及其他I/O函数。

## 1、send & recv函数

send 函数声明

```c
#include <sys/socket.h>

ssize_t send(int sockfd, const void *buf, size_t nbytes, int flags);
// 成功时返回发送的字节数，失败时返回-1

/*
	sockfd 表示数据传输对象连接的套接字文件描述符
	buf 保存待传输数据的缓冲地址值
	nbytes 待传输的字节数
	flags 传输数据时指定的可选项信息
*/
```



recv 函数声明

```c
#include <sys/socket.h>

ssize_t recv(int sockfd, void* buf, size_t nbytes, int flags);
//成功时返回接收的字节数（EOF时返回0），失败时返回-1

/*
	sockfd 表示数据接收对象连接的套接字文件描述符
	buf 保存接收数据的缓冲地址值
	nbytes 可接收的最大字节数
	flag 接收数据时指定的可选项信息
*/
```

send & recv 函数可选项及含义

| 可选项(option) | 含义                                                         | send | recv |
| -------------- | ------------------------------------------------------------ | ---- | ---- |
| MSG_OOB        | 用于传输带外数据(Out-of-band data)                           | Y    | Y    |
| MSG_PEEK       | 验证输入缓冲是否存在接收的数据                               | N    | Y    |
| MSG_DONTROUTE  | 传输传输过程中不参照路由(Routing)表，在本地(Local)网络中寻找目的地 | Y    | N    |
| MSG_DONTWAIT   | 调用I/O函数时不阻塞，用于使用非阻塞(Non-blocking)I/O         | Y    | Y    |
| MSG_WAITALL    | 防止函数返回，直到接收全部请求的字节数                       | N    | Y    |

多个可选项用 |(位或运算符) 连接，例如：  MSG_OOB|MSG_PEEK

不同操作系统对上述可选项的支持也不同，因此，为了使用不同可选项，在实际开发中需要对采用的操作系统有一定了解。



### 1.1 MSG_OOB 发送紧急消息

#### 示例代码

服务器端：

oob_recv.c

```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#define BUF_SIZE 30
void error_handling(char *message);
void urg_handler(int signo);

int acpt_sock;
int recv_sock;

int main(int argc, char *argv[])
{
    struct sockaddr_in recv_adr, serv_adr;
    int str_len, state;
    socklen_t serv_adr_sz;
    struct sigaction act;
    char buf[BUF_SIZE];
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
    act.sa_handler = urg_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    acpt_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&recv_adr, 0, sizeof(recv_adr));
    recv_adr.sin_family = AF_INET;
    recv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    recv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(acpt_sock, (struct sockaddr *)&recv_adr, sizeof(recv_adr)) == -1)
        error_handling("bind() error");
    listen(acpt_sock, 5);

    
    while(1){
        serv_adr_sz = sizeof(serv_adr);
        recv_sock = accept(acpt_sock, (struct sockaddr *)&serv_adr, &serv_adr_sz);
        //将文件描述符 recv_sock 指向的套接字拥有者（F_SETOWN）改为把getpid函数返回值用做id的进程
        fcntl(recv_sock, F_SETOWN, getpid());
        state = sigaction(SIGURG, &act, 0); //SIGURG 是一个信号，当接收到 MSG_OOB 紧急消息时，系统产生SIGURG信号
        while ((str_len = recv(recv_sock, buf, BUF_SIZE-1, 0))!=0)
        {
            if (str_len == -1)
                continue;
            buf[str_len] = 0;
            puts(buf);
        }
        close(recv_sock);
    }
    close(acpt_sock);
    return 0;
}
void urg_handler(int signo)
{
    int str_len;
    char buf[BUF_SIZE];
    str_len = recv(recv_sock, buf, sizeof(buf) - 1, MSG_OOB);
    buf[str_len] = 0;
    printf("Urgent message: %s \n", buf);
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
```

收到MSG_OOB紧急信号时，操作系统将产生SIGURG信号，并调用注册的信号处理函数

fcnt 将文件描述符recv_sock指向的套接字拥有者(F_SETOWN)改为PID为getpid()返回值的进程。



oob_send.c

```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SIZE 30
void error_handling(char *message);

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in recv_adr;
    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }
    sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&recv_adr, 0, sizeof(recv_adr));
    recv_adr.sin_family = AF_INET;
    recv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    recv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&recv_adr, sizeof(recv_adr)) == -1)
        error_handling("connect() error");

    write(sock, "123", strlen("123"));
    send(sock, "4", strlen("4"), MSG_OOB);
    write(sock, "567", strlen("567"));
    send(sock, "890", strlen("890"), MSG_OOB);
    close(sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

```

#### 编译运行

```shell
$ gcc oob_recv.c -o ./bin/oob_recv 
$ ./bin/oob_recv 9190
-----------------
new client : 4
123456789
-----------------
new client : 4
123456789
-----------------
new client : 4
123456789
-----------------
new client : 4
123456789
-----------------
new client : 4
123456789
-----------------
new client : 4
Urgent message: 0 
123
56789
-----------------
new client : 4
123
Urgent message: 0 
56789
-----------------
new client : 4
123456789
-----------------
new client : 4
123456789
-----------------
new client : 4
123456789
-----------------
new client : 4
123
Urgent message: 0 
56789
-----------------
new client : 4
123
Urgent message: 4 
567
Urgent message: 0 
89
-----------------
new client : 4
123456789
-----------------
new client : 4
123456789
-----------------
new client : 4
Urgent message: 0 
123
56789
-----------------
new client : 4
123456789
-----------------
new client : 4
123456789
-----------------
new client : 4
123456789
-----------------
new client : 4
123456789
-----------------
new client : 4
123
567
Urgent message: 0 
89
```

> 有时能产生URG信号，有时不能，MSG_OOB的真正意义在于督促数据接收对象尽快处理数据，如果数据已经接收就不在产生URG信号



```shell
$ gcc oob_send.c -o ./bin/oob_send 
$ ./bin/oob_send 127.0.0.1 9190
$ ./bin/oob_send 127.0.0.1 9190
$ ./bin/oob_send 127.0.0.1 9190
$ ./bin/oob_send 127.0.0.1 9190
$ ./bin/oob_send 127.0.0.1 9190
$ ./bin/oob_send 127.0.0.1 9190
$ ./bin/oob_send 127.0.0.1 9190
。。。
$ ./bin/oob_send 127.0.0.1 9190
```



### 1.2 紧急模式工作原理

**MSG_OOB的真正意义在于督促数据接收对象尽快处理数据**。这是紧急模式的全部内容，而且TCP保持顺序传输"的传输特性依然成立。

使用带外数据的实际程序例子就是telnet,rlogin,ftp命令。前两个程序会将中止字符作为紧急数据发送到远程端。这会允许远程端冲洗所有未处理 的输入，并且丢弃所有未发送的终端输出。这会快速中断一个向我们屏幕发送大量数据的运行进程。ftp命令使用带外数据来中断一个文件的传输。

举例：急珍患者的及时救治需要如下两个条件：

* 迅速入院
* 医院急救

无法快速把病人送到医院，并不意味着不需要医院进行急救。TCP的急救消息无法保证及时入院，但可以要求急救。当然，急救措施由程序员完成。之前的示例oob_recv.c的运行过程中也传递了紧急消息，这可以通过事件处理函数确认。这就是MSG_OOB模式数据传输的实际意义。下面给出设置MSG_OOB可选项状态下的数据传输过程，如下图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-10%20%E4%B8%8B%E5%8D%889.54.06.png" alt="截屏2021-05-10 下午9.54.06" style="zoom:50%;" />

上图给出oob_recv.c调用send(sock, "890", strlen("890"), MSG_OOB)后输出缓冲状态，假设已传输之前的数据。

如果将缓冲最左端的位置视为偏移量为0，字符0保存于偏移量为2的位置。另外，字符0右侧偏移量为3的位置存有紧急指针。紧急指针指向紧急消息的下一个位置（偏移量加一），同时向对方主机传递如下信息：

“紧急指针指向的偏移量为3的之前的部分就是紧急消息！”

也就是，只用一个字节表示紧急消息，这一点可通过上图用于传输数据的TCP数据包的部分结构看的更清楚。如下图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-10%20%E4%B8%8B%E5%8D%8810.23.49.png" alt="截屏2021-05-10 下午10.23.49" style="zoom: 50%;" />

TCP头中含有如下两种信息：

* URG=1:载有紧急消息的数据包
* URG指针：紧急指针位于偏移量为3的位置

指定MSG_OOB选项的数据包本身就是紧急数据包，并通过紧急指针表示紧急消息所在位置，但如图13-2无法得知以下事实：

“紧急消息是字符串890，还是90？如若不是，是否为单个字节0？”

这些都不重要，除紧急指针的前面1个字符，数据接收方将通过调用常用输入函数读取剩余部分。

### 1.3 检查输入缓冲

同时设置MSG_PEEK选项和MSG_DONTWAIT选项，以验证输入缓冲中是否存在接收的数据。MSG_PEEK选项，即使读取了缓冲中的数据也不会删除，加上额外的选项MSG_DONTWAIT，以非阻塞方式验证。

#### 示例代码

peek_recv.c

```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void error_handling(char *message);

int main(int argc, char* argv[]){
    int sock;
    struct sockaddr_in send_adr;
    if(argc!=3){
        printf("Usage %s <IP> <PORT>",argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&send_adr, 0 , sizeof(send_adr));
    send_adr.sin_family = AF_INET;
    send_adr.sin_addr.s_addr = inet_addr(argv[1]);
    send_adr.sin_port = htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr*)&send_adr,sizeof(send_adr)) ==-1 ){
        error_handling("connect() error");
    }

    write(sock, "123", 3);
    close(sock);
    return 0;
}

void error_handling(char * message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}
```



peek_send.c

```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void error_handling(char *message);

int main(int argc, char* argv[]){
    int sock;
    struct sockaddr_in send_adr;
    if(argc!=3){
        printf("Usage %s <IP> <PORT>",argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&send_adr, 0 , sizeof(send_adr));
    send_adr.sin_family = AF_INET;
    send_adr.sin_addr.s_addr = inet_addr(argv[1]);
    send_adr.sin_port = htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr*)&send_adr,sizeof(send_adr)) ==-1 ){
        error_handling("connect() error");
    }

    write(sock, "123", 3);
    close(sock);
    return 0;
}

void error_handling(char * message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}
```



#### 编译运行

```shell
$ gcc peek_recv.c -o ./bin/peek_recv
$ ./bin/peek_recv 9190
Buffering 3 bytes: 123 
Read again: 123 
```

```shell
$ gcc peek_send.c -o ./bin/peek_send
$ ./bin/peek_send 127.0.0.1 9190
```



可见仅发送一次的数据，被服务器端读取了两次，第一次调用设置了MSG_PEEK选项，用于检查，读取后并不删除缓冲区的数据。



## 2、readv & writev 函数

功能：对数据进行整合传输及发送的函数，通过writev函数可以将分散保存在多个缓冲中的数据一并发送，通过readv函数可以由多个缓冲分别接收。因此，适当使用这2个函数可以介绍I/O函数的调用次数。

### 2.1 writev函数

writev函数的声明

```c
#include <sys/uio.h>

ssize_t writev(int filedes, const struct iovec *iov, int iocvnt);
//成功时返回发送的字节数，失败时返回-1

/*
	filedes 文件描述符，可以是套接字，也可以是普通文件描述符
	iov iovec结构体数据的地址值，结构体iovec中包含待发送数据的位置和大小信息
	iovcnt 向第二个参数传递的数组长度
*/
```

iovec结构体

```c
struct iovec {
	void * iov_base; //缓冲地址
  size_t iov_len;  //缓冲大小
}
```

writev函数的功能如下图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-11%20%E4%B8%8A%E5%8D%8811.06.30.png" alt="截屏2021-05-11 上午11.06.30" style="zoom: 67%;" />



#### 示例代码

writev.c

```c
#include <stdio.h>
#include <sys/uio.h>
#include <string.h>

int main(int argc, char * argv[]){
    struct iovec vec[2];
    char buf1[] = "ABCDEF";
    char buf2[] = "123456";

    int str_len;

    vec[0].iov_base = buf1;
    vec[0].iov_len = strlen(buf1);

    vec[1].iov_base = buf2;
    vec[1].iov_len = strlen(buf2);

    str_len = writev(1, vec, 2);
    puts("");
    printf("Write bytes: %d \n", str_len);
    return 0;
}
```



#### 编译运行

```shell
$ gcc writev.c -o ./bin/writev
$ ./bin/writev 
ABCDEF123456
Write bytes: 12 
```



### 2.2 readv函数

```c
#include <sys/uio.h>

ssize_t readv(int filedes, const struct iovec * iov, int iovcnt);
// 成功时返回接收的字节数，失败时返回-1

/*
	filedes 文件描述符，可以是套接字，也可以是普通文件描述符
	包含数据保存位置和大小信息的iovec结构体数组的地址值
	iovcnt 第二个参数中数组的长度
*/
```



#### 示例代码

readv.c

```c
#include <stdio.h>
#include <sys/uio.h>

#define BUF_SIZE 100

int main(int argc, char * argv[]) {
    struct iovec vec[2];
    char buf1[BUF_SIZE] = {0,};
    char buf2[BUF_SIZE] = {0, };
    int str_len;

    vec[0].iov_base  = buf1;
    vec[0].iov_len = 5;

    vec[1].iov_base = buf2;
    vec[1].iov_len = BUF_SIZE;

    str_len = readv(0, vec, 2);
    printf("Read bytes: %d \n", str_len);
    printf("First message: %s \n", buf1);
    printf("Second message: %s", buf2);
    return 0;
}
```



#### 编译运行

```shell
$ gcc readv.c -o ./bin/readv
$ ./bin/readv
I like TCP/IP socket programming~
Read bytes: 34 
First message: I lik 
Second message: e TCP/IP socket programming~
```

Second message已经包含一个换行符，所以不用在printf里输出换行符



### 2.3 合理使用readv & writev 函数

需要传输到的数据位于不同缓冲时，需要多次调用write函数，此时可以通过一次writev调用完成操作，会提高效率。同样，需要将输入缓冲中的数据读入不同位置时，可以不必多次调用read函数，而是利用一次调用readv函数就能大大提高效率。

仅从c语言角度，减少函数调用能相应地提高性能，但其更大意义在于减少数据包个数。假设为了提高效率而在服务器端明确阻止使用Nagle算法。其实writev函数再不采用Nagle算法时更有价值。

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-11%20%E4%B8%8A%E5%8D%8811.40.18.png" alt="截屏2021-05-11 上午11.40.18" style="zoom: 33%;" />

为了提高速度而关闭了Nagle算法，如果调用3次write函数，很有可能通过3个数据包传输，而调用1次writev函数，只需通过1个数据包传输。



需要将不同位置的数据按照发送顺序移动到1个大数组，并通过1次write函数调用进行传输。这种方式，可以直接使用writv函数一次完成。



## 3、习题（参考答案）

（1）下列关于MSG_OOB可选项的说法错误的是？

a. MSG_OOB 指传输 Out-of-band 数据，是通过其他路径高速传输数据

b. MSG_OOB 指通过其他路径高速传输数据，因此 TCP 中设置该选项的数据先到达对方主机

c. 设置 MSG_OOB 是数据先到达对方主机后，以普通数据的形式和顺序读取。也就是说，只是提高了传输速度，接收方无法识别这一点。
d. MSG_OOB 无法脱离 TCP 的默认数据传输方式，即使脱离了 MSG_OOB ，也会保持原有的传输顺序。该选项只用于要求接收方紧急处理。

> 答：b，MSG_OOB的真正意义是督促数据接收对象尽快处理数据，且TCP"保持传输顺序"的传输特性依然成立。
>
> c：MSG_OOB并不会提高传输速度

（2）利用readv & writev函数收发数据有何优点？分别从函数调用次数和I/O缓冲的角度给出说明。

> 答：writev可以将多个缓冲的数据一并发送，readv，可以由多个缓冲一起接收数据。使用readv & writev函数可以减少I/O函数的调用次数。
>
> 当为了提高数据传输速度而关闭Nagle算法时，使用readv & writev函数的优势更加明显，需要发送3个不同缓冲区的内容时，使用write可能需要发送3个数据包，使用writev可能只需发送1个数据包。

（3）通过recv函数验证输入缓冲是否存在数据时（确认后立即返回），如何设置recv函数最后一个参数中的可选项？分别说明各可选项的含义？

> 答：可选项为MSG_PEEK|MSG_DONTWAIT，MSG_PEEK为即使读取了输入缓冲的数据也不会删除，MSG_DONTWAIT为以非阻塞方式验证待读数据存在与否。

