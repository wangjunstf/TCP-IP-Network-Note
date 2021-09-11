# 第 14 章 多播与广播

## 1、多播

### 1.1 多播的数据传输方式及流量方面的优点

多播数据传输的特点

* 多播服务器器端针对特定多播组，只发送一次数据
* 即使只发送一次数据，但改组内的所有客户端都会接收数据
* 多播组数可在IP地址范围内任意增加
* 加入特定组即可接收发往该多播组的数据。

多播组是D类IP地址(224.0.0.0-239.255.255.255）

### 1.2 路由(Routing) TTL(Time to Live, 生存时间) 及加入组的办法

 TTL指数据包传输距离，每经过一个数据包，其值减1，TTL变为0时，将不在被传递



设置TTL通过第9章套接字的可选项完成，与TTL相关的协议层是IPPROTO_IP，选项名为 IP_MULTICAST_TTL，用如下代码设置TTL

```c
int send_sock;
int time_live = 64;
...
send_sock=socket(PF_INET, SOCK_DGRAM, 0);
setsockopt(send_sock, IPPROTO_IP, IP_MULTICAST_TTL, , (void*)&time_live, sizeof(time_live));
...
```



加入多播组也通过设置套接字可选项完成，加入多播组的协议层是IPPROTO_IP，选项名IP_ADD_MEMBERSHIP,通过下列代码加入多播组

```c
int recv_sock;
struct ip_mreq join_adr;
...
recv_sock = socket(PF_INET, SOCK_DGRAM, 0);
...
join_adr.imr_multiaddr.s_addr="多播组地址信息";
join_adr.imr_interface.s_addr="加入多播组的主机地址信息";
setsockopt(recv_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&join_adr, sizeof(join_adr)); 
```



Ip_mreq结构体定义

```c
struct ip_mreq{
  struct in_addr imr_multiaddr;     //加入的组IP地址
  struct in_addr imr_interface;     //加入该组的套接字所属主机的IP地址，也可使用INADDR_ANY
}
```



### 1.3 实现多播 Sender 和 Receiver

该示例的运行场景如下：

* Sender：向AAA组（Broadcastin）文件中保存的新闻信息
* Receiver：接收传递到AAA组的新闻信息



news_sender.c

```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define TTL 64
#define BUF_SIZE 70

void error_handling(char *message);
int main(int argc, char*argv[]){
    int send_sock;
    struct sockaddr_in mul_adr;
    int time_live = TTL;
    int str_len;
    FILE *fp;
    char buf[BUF_SIZE];
    if (argc != 3)
    {
        printf("Usage %s <GroupIP> <PORT>", argv[0]);
        exit(1);
    }

    send_sock = socket(PF_INET, SOCK_DGRAM, 0);
    memset(&mul_adr, 0, sizeof(mul_adr));
    mul_adr.sin_family=AF_INET;
    mul_adr.sin_addr.s_addr = inet_addr(argv[1]);
    mul_adr.sin_port = htons(atoi(argv[2]));

    setsockopt(send_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&time_live, sizeof(time_live));
    if((fp=fopen("news.txt","r")) == NULL )
        error_handling("fopen() error");
    // news.txt 最后一行需多一个换行，不然接收端无法正常输出
    while(1){
        if(fgets(buf,BUF_SIZE-1, fp) == NULL){
            break;
        }
        sendto(send_sock, buf, strlen(buf), 0, (struct sockaddr*)&mul_adr, sizeof(mul_adr));
    }


    fclose(fp);
    close(send_sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
```



news_receive.c

```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define BUF_SIZE 30

void error_handling(char *message);

int main(int argc, char*argv[]){
    int recv_sock;
    int str_len;
    char buf[BUF_SIZE];
    struct sockaddr_in adr;
    struct ip_mreq join_adr;

    if (argc != 3)
    {
        printf("Usage %s <GroupIP> <PORT>", argv[0]);
        exit(1);
    }

    recv_sock=socket(PF_INET, SOCK_DGRAM, 0);
    memset(&adr, 0 , sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_addr.s_addr = htonl(INADDR_ANY);
    adr.sin_port = htons(atoi(argv[2]));

    if(bind(recv_sock, (struct sockaddr*)&adr, sizeof(adr))==-1)
        error_handling("bind() error");
    
    join_adr.imr_multiaddr.s_addr = inet_addr(argv[1]);
    join_adr.imr_interface.s_addr = htonl(INADDR_ANY);

    setsockopt(recv_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&join_adr, sizeof(join_adr));

    while(1){
        str_len = recvfrom(recv_sock, buf, BUF_SIZE-1, 0, NULL, 0);
        if(str_len<0)
            break;
        buf[str_len]=0;
        fputs(buf, stdout);
    }

    close(recv_sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
```

news.txt最后一行要有换行，news_receive先运行，news_sender后运行

编译运行

```shell
$ gcc news_receiver.c -o ./bin/news_receiver
$ ./bin/news_receiver 224.1.1.2 9190
这是1条新闻：
新闻的主要内容为：
one
two
three
2021-1-1
```



```shell
$ gcc news_sender.c -o ./bin/news_sender 
$ ./bin/news_sender 224.1.1.2 9190
```



多播是基于MBone这个虚拟网络工作的，可以将其理解为：“通过网络中的特殊协议工作的软件概念上的网络”。也就是说，MBone并非可以触及的物理网络，它是以物理网络为基础，通过软件方法实现的多播通信必备虚拟网络。



## 2、广播

多播即使跨越不同网络的情况下，只要加入多播组就能接收数据。相反，广播只能向同一网络中的主机传输数据。

### 2.1 广播的理解及实现方法

广播是向同一网络中所有主机传输数据的方法。与多播相同，广播也是基于UDP完成的。根据传输数据时使用IP地址的形式，广播分为如下2种。

* 直接广播（Directed Broadcast）
* 本地广播（Local Broadcast）

直接广播的IP地址：除了网络地址，主机地主全部置为1。例如192.32.24网络中的主机向192.32.24.255传输数据时，数据将传递到192.32.24网络中所有主机。

本地广播使用的IP地址为255.255.255.255，例如，192.32.24网络中的主机向255.255.255.255传输数据时，数据将传递到192.32.24网络中所有主机。



通过修改套接字的可选项，使其支持广播。

```c
int send_sock;
int bcast = 1;    //对变量进行初始化，以将SO_BROADCAST选项信息改为1
...
send_sock = socket(PF_INET, SOCK_DGRAM, 0);
...
setsockopt(send_sock, SOL_SOCKET, SO_BROADCAST, (void*)&bcast, sizeof(bcast));
...

```



### 2.2 实现广播数据的Sender和Receiver

news_sender_brd.c

```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SIZE 30

void error_handling(char *message);

int main(int argc, char*argv[]){
    int send_sock;
    struct sockaddr_in broad_adr;
    FILE *fp;
    char buf[BUF_SIZE];
    int so_brd = 1;
    if (argc != 3)
    {
        printf("Usage %s <GroupIP> <PORT>", argv[0]);
        exit(1);
    }

    send_sock = socket(PF_INET, SOCK_DGRAM, 0);
    memset(&broad_adr, 0, sizeof(broad_adr));
    broad_adr.sin_family = AF_INET;
    broad_adr.sin_addr.s_addr = inet_addr(argv[1]);
    broad_adr.sin_port = htons(atoi(argv[2]));

    setsockopt(send_sock, SOL_SOCKET, SO_BROADCAST, (void *)&so_brd, sizeof(so_brd));
    if ((fp = fopen("news.txt", "r")) == NULL)
        error_handling("fopen() error");
    // news.txt 最后一行需多一个换行，不然接收端无法正常输出
    while (1)
    {
        if (fgets(buf, BUF_SIZE - 1, fp) == NULL)
            break;
        
        sendto(send_sock, buf, strlen(buf), 0, (struct sockaddr *)&broad_adr, sizeof(broad_adr));
    }

    fclose(fp);
    close(send_sock);
    return 0;
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
```



news_receiver_brd.c

```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define BUF_SIZE 30

void error_handling(char *message);

int main(int argc, char *argv[])
{
    int recv_sock;
    int str_len;
    char buf[BUF_SIZE];
    struct sockaddr_in adr;

    if (argc != 2)
    {
        printf("Usage %s <PORT>", argv[0]);
        exit(1);
    }

    recv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    memset(&adr, 0, sizeof(adr));
    adr.sin_family = AF_INET;
    adr.sin_addr.s_addr = htonl(INADDR_ANY);
    adr.sin_port = htons(atoi(argv[1]));

    if (bind(recv_sock, (struct sockaddr *)&adr, sizeof(adr)) == -1)
        error_handling("bind() error");

    while (1)
    {
        str_len = recvfrom(recv_sock, buf, BUF_SIZE - 1, 0, NULL, 0);
        if (str_len < 0)
            break;
        buf[str_len] = 0;
        fputs(buf, stdout);
    }

    close(recv_sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
```



编译运行

news.txt最后一行要有换行，news_receive先运行，news_sender后运行

```shell
$ gcc news_receiver_brd.c -o ./bin/news_receiver_brd
$ ./bin/news_receiver_brd 9190这是1条新闻：
新闻的主要内容为：
one
two
three
2021-1-1
```

```shell
$ gcc news_sender_brd.c -o ./bin/news_sender_brd
$ ./bin/news_sender_brd 255.255.255.255 9190
```



## 3、习题（参考答案）

（1）TTL的含义是什么？请从路由的角度说明较大的TTL值与较小的TTL值之间的区别及问题。

> TTL值指生存时间，即经过路由器的个数。没经过一个路由器，其TTL值减1，直至TTL变为0就不再传输。
>
> 较大的TTL值相比较小的TTL值能传输更远的距离，但太大会影响网络流量。一般设置为64.



（2）多播与广播的异同点是什么？请从数据通信的角度进行说明。

> 答：多播与广播的相同点就是都能用1个数据包，同时向多台主机发送数据。
>
> 不同点：多播即使跨越不同网络的情况下，只要加入多播组就能接收数据。相反，广播只能向同一网络中的主机传输数据。



（3）下列关于多播的描述错误的是？

a. 多播就是用来向加入多播组的所有主机传输数据的协议。

b. 主机连接到同一网络才能加入多播组，也就是说多播组无法跨越多个网络。

c. 能够加入多播组的主机数并无限制，但只能有一个主机（Sender）向该组发送数据。

d. 多播时使用的套接字是UDP套接字，因为多播是基于UDP进行数据通信的。

> 答：b: 不同网络的主机也可以加入同一个多播组
>
> ​        c：可以有多个主机向该组发送数据



（4）多播也对网络流量有利，请比较TCP数据交换方式解释其原因。

> 答：若通过TCP或UDP向1000个主机发送文件，则共需传递1000次，即便将10台主机合并为1个网络，使99%的传输路径相同的情况下也是如此。若使用多播方式传输文件，则只需发送一次。这时由1000台主机构成的网络中的路由器负责复制文件并传递到主机。



（5）多播方式的数据通信需要MBone虚拟网络。换言之，MBone是用于多播的网络，但它是虚拟网络。请解释此处的虚拟网络。

> 答：MBone是“通过网络中的特殊协议工作的软件概念上的网络”。也就是说，MBone并非可以触及的物理网络，它是以物理网络为基础，通过软件方法实现的多播通信必备虚拟网络。

