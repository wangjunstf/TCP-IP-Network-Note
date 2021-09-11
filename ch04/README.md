# 第4章 基于TCP的服务器端/客户端(1)

前面章节讲解了套接字的创建和向套接字分配地址，接下来正式讨论通过套接字收发数据。

## 4.1 理解TCP和UDP

按照数据传输方式不同，套接字可以分为TCP套接字和UDP套接字。TCP是面向连接的，因此又称为基于流（stream）的套接字。

TCP是Transmission Control Protocol(传输控制协议)的简写。



### 4.1.1 TCP/IP协议栈

1. TCP/IP协议栈可用下图表示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/TCP:IP%E5%8D%8F%E8%AE%AE%E6%A0%882.png" alt="TCP:IP协议栈2" style="zoom: 33%;" />

从上图可以看出，面对“基于互联网的有效数据传输”的命题，并非通过1个庞大的协议解决问题，而是化整为零，通过层次化方案——TCP/IP协议栈解决

2. 通过TCP套接字收发数据时需要借助下图这四层。

   <img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/tcp%E5%8D%8F%E8%AE%AE%E6%A0%882.png" alt="tcp协议栈" style="zoom:33%;" />

2. 反之通过UDP套接字收发数据时，利用下图4层协议完成。

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/UDP%E5%8D%8F%E8%AE%AE%E6%A0%882.png" alt="UDP协议栈" style="zoom:33%;" />

各层可能通过操作系统等软件实现，也可能通过类似NIC的硬件实现。

> OSI 7 Layer（层）
>
> 数据通信中使用的协议栈分为7层。对程序员来说掌握4层协议栈就足够了。



### 4.1.2 TCP/IP协议的诞生背景

“通过因特网完成有效数据传输”这个课题让许多专家聚集到了一起，这些人是硬件，系统，路由算法等各领域的顶级专家。仅凭套接字本身并不能解决这个问题，编写软件前需要构建硬件系统，在此基础上需要通过软件实现各种算法。因此，把这个大问题划分成若干小问题再逐个攻破，将大幅提高效率。

> 开放式系统
>
> 有助于资源共享，易于和其它厂商的产品互联



### 4.1.3 链路层

链路层是物理链接领域标准化的结果，也是最基本的领域，专门定义LAN，WAN，MAN等网络标准。若两台主机通过网络进行数据交换，需要通过下图所示物理连接，链路层就负责这些标准。

![网络连接结构](https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E7%BD%91%E7%BB%9C%E8%BF%9E%E6%8E%A5%E7%BB%93%E6%9E%84.png)



### 4.1.4 IP层

准备好物理连接后就要传输数据。为了在复杂的网络中传输数据，首先需要考虑路径的选择，向目标传输数据需要经过哪条途径？解决此问题就是IP层。

IP本身是面向消息的，不可靠的协议。每次传输数据时会帮我们选择路径，但并不一致，如果传输中发生路径错误，则选择其他路径；但如果发生数据丢失或错误，则无法解决。换言之，IP协议无法应对数据错误。



### 4.1.5 TCP/UDP层

TCP和UDP以IP层提供的路径消息为基础完成实际的数据传输，故该层又称传输层。UDP比TCP简单，我们将在后续章节展开讨论，现在只解释TCP。

TCP可以保证可靠的数据传输，但它发送数据时以IP层为基础（这也是协议栈层次化的原因）

IP层只关注一个数据包（数据传输的基本单位）的传输过程。因此，即使传输多个数据包，每个数据包也是由IP层实际传输的，也就是传输数据及传输本身不可靠。利用IP层传输多个数据时，可能接收的数据顺序与发送数据时的顺序不一致，还可能某些数据已经丢失或损坏。

**反之，若添加TCP协议则按照以下对话方式传输数据：**

> 主机A：“正确收到第二个数据包！”
>
> 主机B：“嗯，知道了。”
>
> 主机A：“正确收到第二个数据包！”
>
> 主机B：“可我已发送第四个数据包了啊！哦，您没收到第四个数据包吧？我给您重传！”

以上就是TCP的作用。如果数据交换过程中可以确认对方已收到数据，并重传丢失的数据，那么即便IP层不保证数据传输，这类通信也是可靠的。

![传输控制协议](https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E4%BC%A0%E8%BE%93%E6%8E%A7%E5%88%B6%E5%8D%8F%E8%AE%AE.png)



### 4.1.6 应用层

上述内容是套接字通信过程中自动处理的。选择数据传输路径，数据确认过程都被隐藏到套接字内部。程序员在编程时无需这些过程，但这并不意味着不用掌握这些知识，只有掌握了这些理论，才能编写出符合需求的网络程序。

编写软件的过程，需要根据程序的特点决定服务器端和客户端之间的数据传输规则，这便是应用层协议。网络编程的大部分内容就是编写并实现应用层协议。



## 4.2 实现基于TCP的服务端/客户端

本节实现完整的TCP服务器端，在此过程中各位将理解套接字使用方法及数据传输方法。

### 4.2.1 TCP服务器端的默认函数调用顺序

下图给出了TCP服务器端默认的函数调用顺序，绝大部分TCP服务器端都按照该顺序调用。

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/TCP%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AB%AF%E5%87%BD%E6%95%B0%E8%B0%83%E7%94%A8%E9%A1%BA%E5%BA%8F2.png" alt="TCP服务器端函数调用顺序" style="zoom: 33%;" />



调用socket函数创建套接字，声明并初始化地址信息结构体变量，调用bind函数向套接字分配地址。这2个阶段之前已经讨论过，下面讲解之后几个过程。

### 4.2.2 进入等待连接请求状态

已调用bind函数给套接字分配了地址，接下来就要通过调用listen函数进入等待连接请求状态。只有调用了listen函数，客户端才能进入可发出连接请求的状态。换言之，这时客户端才能调用connect函数(若提前调用将发生错误)

```c
#include <sys/socket.h>

int listen(int sock, int backlog);
//成功时返回0，失败时返回-1
/*
	sock 希望进入等待连接请求状态的套接字文件描述符，传递的描述符套接字参数成为服务器端套接字（监听套接字）
	
	backlog 连接请求等待队列（Queue）的长度，若为5，则队列长度为5，表示最多使5个连接请求进入队列
*/
```

“服务端处于等待连接请求状态”是指，客户端请求连接时，受理连接前一直使请求处于等待状态。如下图：

![等待连接请求状态](https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E7%AD%89%E5%BE%85%E8%BF%9E%E6%8E%A5%E8%AF%B7%E6%B1%82%E7%8A%B6%E6%80%81.png)

上图可知作为listen函数的第一个参数传递的文件描述符套接字的用途。客户端连接请求本身也是从网络中接收到的一种数据，而要想接收就需要套接字，此任务就是由服务器端套接字完成。服务器端套接字是接收连接请求的一名门卫或一扇门。

当客户端向服务器端发送连接请求时，如果系统正忙，则到连接请求等待室等待，服务器准备好后会处理该连接。

**等候室称为连接请求等待队列**

listen函数的第二个参数与服务器的特性有关，像频繁接收请求的Web服务器端至少应为15。另外，连接请求队列的大小始终根据实验结果而定。



### 4.2.3 受理客户端连接请求

下面函数将自动创建套接字，并连接到发起请求的客户端。

```c
#include <sys/socket.h>

int accept(int sock, struct sockaddr *addr, socklen_t* addrlen);
//成功时返回创建的套接字文件描述符，失败时返回-1

/*
	sock 服务器端套接字的文件描述符
	addr 保存发起连接请求的客户端地址信息的变量地址，调用函数后向传递来的地址变量参数填写客户端地址信息
	adrlen 第二个参数addr结构体的长度，但是存有长度的变量地址。函数调用完成后，该变量即被填入客户端地址长度
*/
```

**accept函数受理连接请求等待队列中待处理的客户端连接请求，函数调用成功时，accept函数内部将产生用于数据I/O的套接字，并返回其文件描述符**

具体过程如下图所示：![受理连接请求](https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E5%8F%97%E7%90%86%E8%BF%9E%E6%8E%A5%E8%AF%B7%E6%B1%82.png)



上图展示了“从等待队列中取出1个连接请求，创建套接字并完成连接请求”的过程。服务器端单独创建的套接字与客户端建立连接后进行数据交换。



### 4.2.4 回顾Hello world服务器端

```c
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>

void errorHandling(const char* message);

int main(int argc, char* argv[]){
    if(argc != 2){
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
    if(bind(sockServ, (struct sockaddr*)& sockServAddr, sizeof(sockServAddr)) ==-1){
        errorHandling("bind() error!");
    }
    
    //调用listen函数将套接字转化为可接收连接状态
    if(listen(sockServ, 5) == -1){
        errorHandling("listen() error!");
    }

    struct sockaddr_in sockClientAddr;
    socklen_t clientAddrLen = sizeof(sockClientAddr);
    
    //调用accept函数受理连接请求，如果在没有连接请求的情况下调用该函数，则不会返回，直到有连接请求为止。
    int sockClient = accept(sockServ, (struct sockaddr*)& sockClientAddr, &clientAddrLen);
    if(sockClient == -1){
        errorHandling("accept() error!");
    }
    else{
        puts("New Client connected...");
    }

    char message[] = "Hello World!";
    //write函数用于传输数据，若程序经accept函数运行到本行，说明已经有了连接请求
    write(sockClient, message, strlen(message));

    close(sockClient);
    close(sockServ);

    return 0;
}

void errorHandling(const char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
/*
	服务器实现过程中要先创建套接字，17行创建套接字，此时套接字尚非真正的服务器端套接字。
	20行到28行：为了完成套接字的地址分配，初始化结构体变量并调用bind函数。
	31行：调用listen函数进入等待连接请求状态。连接请求等待队列的长度设置为5，此时的套接字才是服务器端套接字。
	39行：调用accept函数从队头取1个连接请求与客户端建立连接，并返回创建的套接字文件描述符。另外，调用accept函数时若等待队列为空，则accept函数不会返回，直到队列中出现新的客户端连接。
	49行：调用write函数向客户端传输数据。
	51行：关闭与客户端的连接
	52行：关闭服务器端套接字
*/

```



### 4.2.5 客户端的默认函数调用顺序

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/TCP%E5%AE%A2%E6%88%B7%E7%AB%AF%E5%87%BD%E6%95%B0%E8%B0%83%E7%94%A8%E9%A1%BA%E5%BA%8F.png" alt="TCP客户端函数调用顺序" style="zoom:50%;" />

与服务器端相比，区别就在于“请求连接”，它是创建客户端套接字后向服务器端发起的连接请求。同过以下函数发起连接请求。

```c
 #include <sys/socket.h>

int connect(int sock, struct sockadd* servaddr, socklen_t addrlen);
//成功时返回0，失败时返回-1

/*
	sock 客户端套接字文件描述符
	servaddr 保存目标服务器端地址信息的变量地址值
	addrlen 以字节为单位传递给第二个结构体参数servaddr的变量地址长度
*/
```

客户端调用connect函数后，发生以下情况之一才会返回（完成函数调用）

* **服务器端接收连接请求**
* 发生断网等异常情况而中断连接请求

所谓“连接请求”并不意味着服务器端调用accept函数，其实服务器端把连接请求信息记录到等待队列。**因此connect函数返回后并不立即进行数据交换**

**客户端的IP地址和端口在调用connect函数时自动分配，无需调用标记的bind函数进行分配**



### 4.2.6 回顾Hello world客户端

```c
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>

void errorHandling(const char* message);

int main(int argc, char* argv[]){
    if(argc != 3){
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
    if(connect(sock, (struct sockaddr*)&sockAddr, sizeof(sockAddr)) == -1){
        errorHandling("connect() error!");
    }

    char buf[32];
    int readLen = read(sock, buf, sizeof(buf)-1);
    if(readLen > 0){
        buf[readLen] = 0;
        printf("Message from server: %s \n", buf);
    }
    close(sock);
    return 0;
}

void errorHandling(const char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

/*
	第19行：创建准备连接服务器的套接字，此时创建的是TCP套接字。
	第21-25行：结构体变量sockAddr中初始化IP地址和端口信息。初始化值为目标服务器端套接字的IP和端口信息。
	第28行：调用connect函数向服务器端发送连接请求
	第33行：完成连接请求后，接收服务器端传输的数据。
	第38行：接收数据后调用close函数关闭套接字，结束与服务器端的连接。
*/
```



### 4.2.7 基于TCP的服务器端/客户端函数调用关系

前面讲解了TCP服务器端/客户端的实现顺序，实际二者并非相互独立，它们的调用关系可以用下图表示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-04-15%20%E4%B8%8B%E5%8D%888.28.16.png" alt="截屏2021-04-15 下午8.28.16" style="zoom:50%;" />



总体流程整理如下：

服务器端创建套接字后连续调用bind，listen函数进入等待状态，客户端通过调用connect函数发起连接请求。需要注意的是，客户端只能等到服务器端调用listen函数才能调用connect函数。

同时要清楚，客户端调用connect函数前，服务器端有可能率先调用accept函数。当然，此时服务器端调用accept函数进入阻塞（blocking）状态，知道客户端调用connect函数为止。



## 4.3 实现迭代服务器端/客户端

本节编写回声(echo)服务器端/客户端。顾名思义，服务器端将客户端传输的字符串数据原封不动地传回客户端，就像回声一样。

### 4.3.1 实现迭代服务器端

之前讨论的Hello world服务器端处理完1个客户端连接请求就退出了，连接请求等待队列实际没有太大意义。但这并非我们想象的服务器端。设置好等待队列的大小后，应向所有客户端提供服务。如果想继续受理后续的客户端连接请求，应该怎样扩展代码？最简单的办法就是插入循环语句反复调用accept函数，如下图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E8%BF%AD%E4%BB%A3%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AB%AF%E8%B0%83%E7%94%A8%E9%A1%BA%E5%BA%8F-20210910221452466.png" alt="迭代服务器端调用顺序" style="zoom:50%;" />

从上图可知，调用accept函数后，紧接着调用I/O相关的read，write函数。然后调用close函数，这并非针对服务器端套接字，而是针对accept函数调用时创建的套接字。

调用close函数就意味着结束了针对某一客户端的服务。如果此时还想服务于其他客户端，就要重新调用accept函数。

**在学习进程和线程前，一个套接字同一时刻只能服务于一个客户端**



### 4.3.2 迭代回声服务器端/客户端

首先整理下程序的基本运行方式：

* 服务器端在同一时刻只与一个客户端相连，并提供回声服务。
* 服务器端依次向5个客户端提供服务并退出。
* 客户端接收用户输入的字符串并发送给服务端。
* 服务器端将接收的字符串数据传回给客户端，即“回声”。
* 服务器端与客户端之间的字符串回声一直执行到客户端输入Q为止。

echo_server.c

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
    int serv_sock,clnt_sock;
    char message[BUF_SIZE];

    struct sockaddr_in serv_adr,clnt_adr;
    socklen_t clnt_adr_sz;
    if (argc != 2)
    {
        printf("Usage: %s <port> \n", argv[1]);
        exit(1);
    }

    //调用socket函数创建套接字
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock==-1){
        errorHandling("socket() error");
    }
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    //调用bind函数分配IP地址和端口号
    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
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
            puts("connected client %d \n",i+1);
        while ((str_len = read(clnt_sock, message, BUF_SIZE)))
            //str_len表示读取到的字符串长度
            write(clnt_sock, message, str_len);
        close(clnt_sock);
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

> 第49到64行：为处理5个客户端连接而添加的循环语句。共调用5次accept函数，依次向5个客户端提供服务
>
> 第57行和59行，实际完成回声服务的代码，原封不动地传输读取的字符串。
>
> 第60行close函数，向连接的相应套接字发送EOF。换言之，客户端套接字若调用close函数，则第57行的循环条件变为假，因此执行第53行的代码
>
> 第62行：向5个客户端提供服务后关闭服务的套接字并终止程序

运行结果：echo_server.c

```shell
gcc echo_server.c -o echo_server #编译
./echo_server 9190               #运行
#每次客户端连接，都答应当前编号
Connected client 1
Connected client 2
Connected client 3
Connected client 4
Connected client 5
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

int main(int argc,char* argv[]){
    int sock;
    char message[BUF_SIZE];
    int str_len;
    struct sockaddr_in serv_adr;
    
    if(argc!=3){
        printf("Usage : %s <IP> <port>\n",argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock==-1){
        errorHandling("socket() error");
    }

    memset(&serv_adr,0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1){
        errorHandling("connect() error!");
    }else{
        puts("Connected......");
    }

    while(1){
        fputs("Input message(Q to quit):",stdout);
        fgets(message, BUF_SIZE, stdin);
        if(!strcmp(message,"q\n") || !strcmp(message, "Q\n"))
            break;

        write(sock, message, strlen(message));
        str_len = read(sock, message, BUF_SIZE-1);
        message[str_len] = 0;
        printf("Message from server: %s",message);
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

> 第32行：调用connect函数。若调用该函数引起的请求连接被注册到服务器端等待队列，则connect函数将完成正常调用。因此，即使通过第35行代码输出了连接提示字符串——如果服务器尚未调用accept()函数——也不会真正建立服务关系

运行结果：echo_client.c



```shell
gcc echo_server.c -o echo_server #编译
./echo_server 127.0.0.1 9190
#以下为终端输出
Connected......
Input message(Q to quit):hello world
Message from server: hello world
Input message(Q to quit):hi! 
Message from server: hi!
Input message(Q to quit):q

```



### 4.3.3 回声客户端存在的问题

```c
write(sock, message, strlen(message));
str_len = read(sock, message, BUF_SIZE-1);
message[str_len] = 0;
printf("Message from server: %s",message);
```

”以上代码有个错误假设，每次调用read，write函数时都会以字符串为单位执行实际的I/O操作。“

因为客户端是基于TCP的，其传输的数据不存在数据边界，因此，多次调用write函数传递的字符串有可能一次性传递到服务器。

本代码中看似结果完全正确，其实这是巧合，在实际数据量大时仍存在发生错误的可能。



## 4.4 习题（参考答案）

（1）请说明TCP/IP的4层协议栈，并说明TCP和UDP套接字经过的层级结构差异。

> 答：从上到下依次为应用层，（TCP层｜UDP层），IP层，数据链路层。
>
> TCP：传输数据经过：应用层，TCP层，IP层，数据链路层
>
> UDP：传输数据经过：应用层，UDP层，IP层，数据链路层
>
> 

（2）请说出TCP/IP协议栈中链路层和IP层的作用，并给出二者关系。

> 答：链路层的作用：在两个网络实体之间提供数据链路连接的创建、维持和释放管理。构成数据链路数据单元（frame：数据帧或帧），并对帧定界、同步、收发顺序的控制。传输过程中的网络流量控制、差错检测和差错控制等方面。只提供导线的一端到另一端的数据传输。[参考wiki](https://zh.wikipedia.org/wiki/%E6%95%B0%E6%8D%AE%E9%93%BE%E8%B7%AF%E5%B1%82)
>
> IP数据包包含源地址，目的地址，和所要传输的信息。该数据包通过链路层传递。



（3）为何需要把TCP/IP协议栈分成4层或7层？结合开放式系统回答。

> 答：因为网络数据传输是一个复杂的课题，通过将网络数据传输分层，一方面可以简化系统，提高系统的扩展能力；另一方面，也有利于标准化的建立，通过标准化，不同公司的产品都可以保证稳定工作，从而促进技术的高速发展。



（4）客户端调用connect函数向服务器端发送连接请求。服务器端调用哪个函数后，客户端可以调用connect函数？

> 答：服务器端调用listen函数后，转为可接听状态，此时客户端可以调用connect函数。



（5）什么时候创建连接请求等待队列？它有何作用？与accept有什么关系？

> 答：服务器端调用listen函数后创建连接请求等待队列
>
> 作用是：当服务器端工作繁忙时（有其他客户端正在与服务器端连接），此时客户端就进入连接请求等待队列。
>
> 与accept的关系：服务器端调用accept函数，从队头取1个连接请求与客户端建立连接。



（6）客户端为何不需要调用bind函数分配地址？如果不调用bind函数，那何时，如何向套接字分配IP地址和端口号？

> 答：客户端调用connect函数时，IP地址和端口号都由操作系统内核自动分配，其中IP地址为本机IP，端口号随机。

（7）把第1章的hello_server.c和hello_server_win.c改成迭代服务器，并利用客户端测试更改是否准确。

代码参见：hello_server.c