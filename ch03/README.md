# 第3章 地址族与协议族

本章着重讲解给套接字分配IP地址和端口号。

## 3.1 分配给套接字的IP地址和端口号

IP时Internet Protocol（网络协议的简称），是为收发网络数据而分配给计算机的地址值。端口号并非赋予计算机的值，而是为了区分程序中创建的套接字而分配给套接字的序号。下面逐一讲解。

### 3.1.1 网络地址

为使计算机连接到网络并收发数据，必须向其分配IP地址。IP地址分为两类。

- IPv4（Internet Protocol version 4）4字节地址族
- IPv6 （Internet Protocol version 6）16字节地址族

IPv4和IPv6的差别主要是表示IP地址所用的字节数，目前通用的地址族为IPv4，IPv6是为了应对2010年前后IP地址耗尽的问题而提出的标准，即便如此，现在还是主要使用IPv4。IPv6的普及还需要很长时间。



IPv4标准的4字节地址分为网络地址和主机（指计算机）地址，且分为A，B，C，D，E等类型。一般不使用已被预约了的E类地址，故省略。

![IPv4地址族](https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/IPv4%E5%9C%B0%E5%9D%80%E6%97%8F.png)

网络地址是为区分网络而设置的一部分IP地址。假设向www.semi.com公司传输数据，该公司内部构件了局域网，把所有计算机连接起来。因此，首先应向SEMI.COM网络传输数据。也就是说，并非一开始就浏览所有的4字节数据，进而找到目标主机；而是仅浏览4字节IP地址的网络地址，先把数据传到SEMI.COM的网络。SEMI.COM的网络（构成网络的路由器）接收到数据后，浏览传输数据的主机地址并将数据传给目标计算机。

下图展示了这个过程：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E5%9F%BA%E4%BA%8EIP%E5%9C%B0%E5%9D%80%E7%9A%84%E6%95%B0%E6%8D%AE%E4%BC%A0%E8%BE%93%E8%BF%87%E7%A8%8B.png" alt="基于IP地址的数据传输过程" style="zoom: 67%;" />



某主机向203.211.172.103和203.211.217.202传输数据，其中203.211.172和203.211.217为该网络的网络地址（稍后将给出网络地址的区分方法）。所以，“向相应网络传输数据“实际上是向构成网络的路由器(Router)或交换机(Switch)传递数据，由接收数据的路由器根据数据中的主机地址向目标地址主机传递数据。

> 路由器和交换机
>
> 若想构建网络，需要一种物理设备完成外网与本网主机之间的数据交换，这种设备就是路由器或交换机。它们其实也是一种计算机，只是为特殊目的而设计运行的，因而有了别名。如果在我们使用的计算机上安装适当的软件，也可以将其用作交换机。另外，交换机比路由器功能要简单一些，而实际用途差不多。

### 3.1.2 网络地址分类与主机地址边界

只需通过IP地址的第一个字节即可判断网络地址占用的字节数。

- A类地址的首字节范围：0～127
- B类地址的首字节范围：128～191
- C类地址的首字节范围：192～223

还有如下表述方式

- A类地址的首字节范围以0开始（指二进制000000）
- B类地址的首字节范围以10开始（指二进制10000000）
- C类地址的首字节范围以110开始（指二进制11000000）

因此通过套接字传输数据时，数据传到网络后即可轻松找到正确的主机。

例如：上述IP地址203.211.172.103

通过第一个字节的值为203，代表其是C类地址，而C类地址的前3个字节为网络地址，后1个字节为主机地址。因此可以立即知道其网络地址203.211.172，主机地址为103

> 一个路由器构成一个网络，该路由器也是该网络的一台主机，也有IP地址。上述IP地址的网络地址203.211.172代表某个路由器构成的网络，传输数据时，要先找到路由器，由路由器把数据传递给相应的主机,那么怎么找到路由器，==路由器的IP地址一般为主机位为1，例如上述IP地址的路由器地址为203.211.172.1==



### 3.1.3 用于区分套接字的端口号

IP地址用于区分计算机，只要有IP地址就可以向目标主机传输数据，假如一台计算机只能运行一个套接字，那么没问题，计算机只需把接收到的数据发送给该套接字就可以。但事实却是，一台计算机往往存在多个套接字，为了区分数据是传递给哪台主机的哪个套接字，只有IP地址是不够的，还需要端口号，一个端口号对应一个套接字。因此IP地址+端口号，就可以给指定计算机的指定端口号发送数据了。

计算机中一般配有NIC（Network InterFace Card, 网络接口卡）数据传输设备。通过NIC向计算机内部传输数据时会用到IP。操作系统负责把传递到内部的数据适当分配给套接字，这时就要利用端口号。也就是说，通过NIC接收的数据内有端口号，操作系统正是参考次端口号把数据传输给相应的套接字。

![数据分配过程](https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%95%B0%E6%8D%AE%E5%88%86%E9%85%8D%E8%BF%87%E7%A8%8B.png)

端口号由16位构成，可分配的端口号范围为：0到65535。但0到1203是知名端口（Well-know PORT），一般分配给特定应用程序，所以应当分配次范围外的值。另外，TCP套接字和UDP套接字可以使用同样的端口号，并不会冲突。



## 3.2 地址信息的表示

应用程序中使用的IP地址和端口号以结构体的形式给出了定义。本节将以IPv4为中心，围绕次结构体讨论目标地址的表示方法。



### 3.2.1 表示IP地址的结构体

填写地址信息时应以如下题问为线索进行：

> 问题1:“采用哪一种地址族？”
>
> 答案1:“基于IPv4的地址族。”

> 问题2：“IP地址是多少？”
>
> 答案2:“211.204.214.76。”

> 问题3:“端口号是多少？”
>
> 答案3:“2048。”

定义以下结构体就可以上述问题：

```c
struct sockaddr_in{
  sa_family_t 		sin_family;    //地址族（Address Family）
  uint16_t    		sin_port;			 //16位TCP/UDP端口号
  struct in_addr  sin_addr;			 //32位IP地址
  char 						sin_zero[8]     //不使用
};

struct in_addr{
  in_addr_t  			s_addr;         //32位IPv4地址
};
```

理解uint16_t，in_addr_t等类型可以参考POSIX（Portable Operating System Interface，可移植操作系统接口）。POSIX是为UNIX系列操作系统设立的标准，它定义了一些其它数据类型。

下表为POSIX中定义的数据类型

| 数据类型名称 | 数据类型说明                        | 声明的头文件 |
| ------------ | ----------------------------------- | ------------ |
| int8_t       | signed 8-bit int                    | sys/types.h  |
| uint8_t      | unsigned 8-bit int(unsigned char)   | sys/types.h  |
| int16_t      | signed 16-bit int                   | sys/types.h  |
| uint16_t     | unsigned 16-bit int(unsigned short) | sys/types.h  |
| int32_t      | signed 32-bit int                   | sys/types.h  |
| uint32_t     | unsigned 32-bit int(unsigned long)  | sys/types.h  |
| sa_family_t  | 地址族（address family）            | sys/socket.h |
| socklen_t    | 长度(length of struct)              | sys/socket.h |
| in_addr_t    | IP地址，声明为uint32_t              | netinet/in.h |
| in_port_t    | 端口号，声明为uint16_t              | netinet/in.h |

> 之所以需要额外定义这些数据类型？
>
> 考虑到扩展性的结果。如果使用int32_t类型的数据，就能保证在任何时候都占用4字节，即使将来用64位表示int类型也是如此。



### 3.2.2 结构体sockaddr_in的成员分析

接下来重点观察结构体成员的含义及其包含的信息

**成员sin_family**

每种协议族适用的地址族均不同，比如IPv4使用4字节地址族，IPv6使用16字节地址族。可以参考下表保存sin_family地址信息。

| 地址族（Address Family） | 含义                             |
| ------------------------ | -------------------------------- |
| AF_INET                  | IPv4网络协议中使用的地址族       |
| AF_INET6                 | IPv6网络协议中使用的地址族       |
| AF_LOCAL                 | 本地通信中采用的UNIX协议的地址族 |

AF_LOCAL只是为了说明具有多种地址族而添加，希望各位不要太感到突然。

**成员sin_port**

该成员保存16位端口号，重点在于，**它以网络字节序保存**（关于这一点稍后将给出详细说明）。

**成员sin_addr**

该成员保存16位端口号，重点在于，它以网络字节序保存。为理解好该成员，应同时观察结构体in_addr。但结构体in_addr声明为uint32_t，因此只需当作32位整数型即可。

**成员sin_zero**

无特殊含义。只是为了使结构体sockaddr_in的大小与sockaddr结构体保持一致而插入的成员。必须填充为0，否则无法得到想要的结果。后面还会讲解sockaddr

sockaddr_in结构体变量地址值将以如下方式传递给bind函数。稍后将给出关于bind函数的详细说明，希望各位重点关注参数传递和类型转换部分代码

```c
struct sockaddr_in serv_addr;
....
if(bind(serv_addr, (struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1)
  error_handling("bind() error");
....
```

此处重要的是第二个参数的传递，在调用bind函数前，先创建sockaddr_in结构体，可以生成符合bind函数要求的字节流。最后将其转换为sockaddr型的结构体变量，再传递给bind函数即可。



> sockaddr_in已经指定了IPV4，为啥sockaddr结构体之所以还需要指定地址族？
>
> 因为sockaddr并非只为IPv4设计。



## 3.3 网络字节序与地址变换

不同的CPU中，四字节整数型值1在内存空间的保存方式可能不同。

四字节整数型值1可用二进制表示如下：

00000000 00000000 00000000 00000001

有些CPU以上述方式保存，另一些CPU可能以倒序保存，例如：

00000001 00000000 00000000 00000000

若不考虑这个问题，数据的收发可能会出现问题，因为保存顺序的不同，意味着对接收数据的解析顺序也不同。



### 3.3.1 字节序（Order）与网络字节序

CPU向内存保存数据的方式有两种，意味着CPU解析数据的方式也分为两种。

- 大端序（Bid Endian）:高位字节存放到低位地址
- 小端序（Little Endian）:高位字节存放到高位地址

假设内存地址由0x20到0x23。要在内存中存储整数0x12 0x34 0x56 0x78

大端序，内存由低地址到高地址依次存储：0x12 0x34 0x56 0x78  

小端序，内存由低地址到高地址依次存储：0x78 0x56 0x34 0x12



每种CPU的数据保存方式可能不同。因此，代表CPU数据保存方式的**主机字节序（Host Byte Order）**在不同CPU中也各不相同。目前主流的Intel系列CPU以小端序方式保存数据。



某台计算机A以大端序向一台小端序计算机B发送数据：0x34 0x12，计算机B解析到的数据0x12 0x34。因字节序的不同，导致接收方收到了错误的数据。

为了解决上述问题，在通过网络传输数据时约定了统一方式，这种约定称为**网络字节序**（Network Byte Order）,非常简单，统一为大端序。

即：先把数据数组转化成大端序格式再进行网络传输。因此，所有计算机接收数据时应识别该数据是网络字节序，小端序系统传输数据时应转化为大端序排列方式。

### 3.3.2 字节序转换

相信大家已经理解了为何要在填充sockadr_in结构体前将数据转换为网络字节序。接下来介绍帮助转换字节序的函数。

- unsigned short htons(unsigned short);
- unsigned short ntohs(unsigned short);
- unsigned long htonl(unsigned long);
- unsigned long ntohl(unsigned long);

上述函数中

htons中的h代表主机（host）字节序，n代表网络（network）字节序，s指short，l指long（==Linux中long类型占用4字节，这很关键==）

因此：h，to，n，s的组合，也可以解释为：“把short类型数据从主机字节序转化为网络字节序。”

同样：ntohs，可以解释为：“把short型数据从网络字节序转化为主机字节序。”



下面程序演示字节序转换过程：

endian_conv.c

```c
#include <stdio.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]){
    unsigned short host_port = 0x1234;
    unsigned short net_port;

    unsigned long host_addr = 0x12345678;
    unsigned long net_addr;

    //将net_port,net_addr转化为网络字节序
    net_port = htons(host_port); 
    net_addr = htonl(host_addr);

    printf("主机字节序端口号：%#x\n",host_port);
    printf("网络字节序端口号：%#x\n",net_port);

    printf("主机字节序地址：%#lx\n",host_addr);
    printf("网络字节序地址：%#lx\n",net_addr);
    return 0;
}
```



**编译**

gcc endian_conv.c -o  endian_conv

**运行**

./endian_conv

**运行结果**

在终端输出：

```shell
主机字节序端口号：0x1234
网络字节序端口号：0x3412
主机字节序地址：0x12345678
网络字节序地址：0x78563412
```



> 除了向sockaddr_in结构体变量填充数据外，其他情况无需考虑字节序问题。



## 3.4 网络地址的初始化与分配

接下来介绍以bind函数为代表的结构体的应用

### 3.4.1 将字符串信息转换为网络字节序的整数型

sockaddr_in中保存地址信息的成员为32位整数型，因此为了分配IP地址，需要将我们熟悉的字符串类型，转换为4字节整数型数据。

对于IP地址，我们熟悉的是点分十进制表示法，有一个函数可以帮我们把字符串形式的IP地址转换成32位整数型数据，此函数在转换类型的同时进行网络字节序转换。

```c
#include <arpa/inet.h>
in_addr_t inet_addr(const char* string);
//成功时返回32位大端序整数型值
//失败时返回INADDR_NONE
```

下面展示该函数的调用过程：

代码参见：inet_addr.c

```c
#include <stdio.h>
#include <arpa/inet.h>

int main(int argc,char* argv[]){
    char* addr1= "1.2.3.4";
    char* addr2 = "1.2.3.256";

    unsigned long conv_addr = inet_addr(addr1);
    if(conv_addr==INADDR_NONE){
        printf("Error occured!");
    }else{
        printf("Network ordered integer addr: %#lx \n",conv_addr);
    }
    conv_addr = inet_addr(addr2);
    if (conv_addr == INADDR_NONE)
    {
        printf("Error occured!");
    }
    else
    {
        printf("Network ordered integer addr: %#lx \n", conv_addr);
    }

    return 0;
}
```



**编译**

gcc inet_addr.c -o inet_addr

**运行**

./inet_addr

**输出**

Network ordered integer addr: 0x4030201 
Error occured

> addr1转换成功，一个字节的最大整数为255，所以addr2转换失败了。



inet_aton函数与inet_addr函数在功能上完全相同，也将字符串形式IP地址信息带入sockaddr_in结构体中声明的in_addr结构体变量。而inet_aton函数则不需要此过程，因为它会在转换后，再把结果填入该结构体变量。

**下列代码展示inet_aton的调用过程**

inet_aton.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

void errorHandling(const char *message);

int main(int argc, char* argv[]){
    char* addr = "127.232.124.79";
    struct sockaddr_in addr_inet;
    
    //IP地址转换后，会自动保存到in_addr结构体里，省去了手动保存IP地址信息的过程
    if(!inet_aton(addr,&addr_inet.sin_addr)){
        errorHandling("Conversion error");
    }else{
        printf("Network ordered integer addr: %#x\n" ,addr_inet.sin_addr.s_addr);
    }
    return 0;
}
void errorHandling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
```



**编译**

gcc inet_aton.c -o inet_aton

**运行** 

./inet_aton

**运行结果**

Network ordered integer addr: 0x4f7ce87



下面函数执行与inet_aton函数正好相反的函数，此函数可以把网络字节序整数型IP地址转换成为我们熟悉的字符串形式。

```c
#include <arpa/inet.h>

char* inet_ntoa(struct in_addr adr);
//成功时返回转换的字符串地址值，失败时返回-1
```

> 调用完该函数，应该立即将字符串信息复制到其他内存空间，否则再次调用该函数时，之前的字符串就会被覆盖。

下列代码展示inet_ntoa的调用过程

```c
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    struct sockaddr_in addr1, addr2;
    char *str_ptr;
    char str_arr[20];

    addr1.sin_addr.s_addr = htonl(0x1020304);
    addr2.sin_addr.s_addr = htonl(0x1010101);
    //把addr1中的结构体信息转换为字符串的IP地址形式
    str_ptr = inet_ntoa(addr1.sin_addr);
    strcpy(str_arr, str_ptr);
    printf("Dotted-Decimal notation1: %s \n", str_ptr);

    inet_ntoa(addr2.sin_addr);
    printf("Dotted-Decimal notation2: %s \n", str_ptr);
    printf("Dotted-Decimal notation3: %s \n", str_arr);
    return 0;
}
```



**编译**

gcc inet_aton.c -o inet_aton

**运行** 

./inet_ntoa.c

**运行结果**

Dotted-Decimal notation1: 1.2.3.4 
Dotted-Decimal notation2: 1.1.1.1 
Dotted-Decimal notation3: 1.2.3.4 



### 3.4.2 网络地址的初始化

现在介绍套接字创建过程中常见的网络地址信息初始化方法

```c
struct sockaddr_in addr;
char* serv_ip = "211.217.168.13";     //声明IP地址字符串
char* serv_port = "9190";             //声明端口号字符串
memset(&addr, 0 ,sizeof(addr));       //结构体变量addr的所有成员初始化为0
addr.sin_family = AF_INET;            //指定地址族
addr.sin_addr.s_addr = inet_addr(serv_ip);  //基于字符串的IP地址初始化
addr.sin_port = htons(atoi(serv_port));     //基于字符串的端口号初始化
```

上述代码对IP地址和端口号进行了硬编码，这并非良策，因为运行环境改变就得更改代码。因此我们运行示例main函数时传输IP地址和端口号。



### 3.4.3 客户端地址信息初始化

给套接字分配IP地址和端口号主要是为了下面两件事做准备：

**对于服务器端：**

“请把进入IP 211.217.168.13 ，9190端口的数据给我。”

对于客户端连接请求：

“请连接到IP211.217.168.13，9190端口！”



> 服务器端的准备工作通过bind函数完成，而客户端则通过connect函数完成 

### 3.4.4 INADDR_ANY

```c
struct sockaddr_in addr;
char* serv_port = "9190";
memset(&addr, 0, sizeof(addr));
addr.sin_family = AF_INET;
addr.sin_addr.s_addr = htonl(INADDR_ANY);
addr.sin_port = htons(atoi(serv_port));
```

利用常数 INADDR_ANY 分配服务器端的IP地址，采用这种方式，则可自动获取运行服务器端的计算机IP地址，不必亲自输入。若同一计算机中已分配多个IP地址（多宿主(Multi-homed)计算机，一般路由器属于这类），则只要端口号一致，就可以从不同IP接收数据。因此，服务器端中优先考虑这种方式。客户端中除非带有一部分服务器端功能，否则不会采用。

> 初始化服务器端为什么要提供IP地址？
>
> 因为一台计算机可以分配多个IP地址，实际IP地址的个数与计算机中安装的NIC的数量相等。即使是服务器端套接字，也需要决定应接收哪个IP传来的数据。另外，若只有一个NIC，则可以直接使用INADDR_ANY



## 3.5 第一章的hello_server.c    hello_client.c运行过程

运行服务器端：[hello_server.c](https://github.com/wangjunstf/TCP-IP-Network-Note/blob/main/ch01-%E7%90%86%E8%A7%A3%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B%E5%92%8C%E5%A5%97%E6%8E%A5%E5%AD%97/src/hello_server.c)

./hserver 9190

只需指定端口号，而无需指定IP地址，因为可以通过 INADDR_ANY指定IP地址



运行相当于客户端的[hello_client.c](https://github.com/wangjunstf/TCP-IP-Network-Note/blob/main/ch01-%E7%90%86%E8%A7%A3%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B%E5%92%8C%E5%A5%97%E6%8E%A5%E5%AD%97/src/hello_client.c)

./hclient 127.0.0.1 9190

127.0.0.1指回送地址（loopback address）,指的是计算机自身的IP地址。



### 3.5.1 向套接字分配网络地址

既然已经讨论了sockaddr_in结构体的初始化方法，接下来就把初始化的地址信息分配给套接字。bind函数完成这项工作。

```c
#include <sys/socket.h>

int bind(int sockfd, struct sockaddr* myaddr, socklen_t addrlen);
//成功时返回0，失败时返回-1。
/*
	sockfd 要分配地址信息（IP地址和端口号）的套接字文件描述符
	myaddr 存有地址信息的结构体变量地址值
	addrlen 第二个结构体变量的长度
*/
```



下面给出服务器常见套接字初始化过程

```c
int serv_sock;
struct sockaddr_in serv_addr;
char* serv_port = "9190";

/*创建服务器端套接字（监听套接字）*/
serv_sock = socket(PF_INET, SOCK_STREAM, 0);

/*地址信息初始化*/
memset(&serv_addr, 0, sizeof(serv_addr));
serv_addr.sin_family = AF_INET;
serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
serv_addr.sin_port = htons(atoi(serv_port));

/*分配地址信息*/
bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
```



服务器端代码结构默认如上，当然还有未显示的异常处理代码。



## 3.6习题（参考答案）

（1）IP地址族 IPv4 与 IPv6 有什么区别？在何种背景下诞生了 IPV6?

> 答：IPv4是4字节地址族，IPv6是16字节地址族。最大的区别是可表示的IP地址总数不同。
>
> IPv6的提出主要是为了应对2010年前后IP地址耗尽的问题。

（2）通过IPv4网络ID，主机ID及路由器的关系说明向公司局域网中的计算机传输数据的过程。

> 答：主机A向公司局域网内的计算机发送数据时，首先通过IP地址得到该公司局域网的网络ID，通过该网络ID可以找到路由器地址，数据会转发给路由器，路由器根据该IP的主机ID，再把数据转发给目标主机。

（3）套接字地址分为IP地址和端口号。为什么需要IP地址和端口号？或者说，通过IP可以区分哪些对象？通过端口号可以区分哪些对象？

> 答：通过IP地址可以区分不同的主机，在一台计算机上，若存在多块NIC(Network InterFace Card, 网络接口卡)，则IP地址还可以区分不同的NIC。
>
> 端口号可以区分一台计算机上的不同套接字，即应用程序。

（4）请说明IP地址的分类方法，并据此说出下面这些IP地址的分类。

> 根据IP地址的首字节范围，可以确定IP地址是A类，B类还是C类。
>
> - A类地址的首字节范围：0～127
> - B类地址的首字节范围：128～191
> - C类地址的首字节范围：192～223

- 214.121.212.102   (C类地址)
- 120.101.122.89     (A类地址)
- 129.78.102.211     (B类地址)

（5）计算机通过路由器或交换机连接互联网。请说出路由器和交换机的作用？

> 答：路由器和交换机完成外网和本网之间的数据交换。
>
> 路由器用于连接不同的网络，交换机主要是能为子网络中提供更多的连接端口，连接更多的计算机。
>
> 一台交换机要能上网，必须连接路由器。

更多路由器与交换机的区别：

请参考：[路由器和交换机的区别 - Wangjun’Blog，一个极客的成长之路](http://www.wangjunblogs.com/index.php/archives/16/)

https://www.cnblogs.com/Lynn-Zhang/articles/5754336.html



（6）什么是知名端口？其范围是多少？知名端口号中具有代表性的HTTP和FTP端口号各是多少？

> 答：分配给特定应用程序的端口号为知名端口号。0到1203是知名端口。
>
> HTTP：80   FTP:21

（7）向套接字分配地址的 bind 函数原型如下：

```
int bind(int sockfd, struct sockaddr *myaddr, socklen_t addrlen);
```

而调用时则用：

```
bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)
```

此处 serv_addr 为 sockaddr_in 结构体变量。与函数原型不同，传入的是 sockaddr_in 结构体变量，请说明原因。

> 答：bind函数的第二个参数期望得到sockaddr结构体变量地址值，包括地址族，端口号，IP地址等。直接向sockaddr填充这些信息会带来麻烦，继而有了sockaddr_in结构体，该结构体大小和sockaddr结构体一致，填写完sockaddr_in后，其二进制数据和sockaddr一致，只是地址类型不一样。此时只需把sockaddr_in的地址类型转换为sockaddr的地址类型就可以。

（8）请解释大端序，小端序，网络字节序，并说明为何需要网络字节序。

> 答：大端序：高位字节存放到低位地址
>
> ​        小端序：高位字节存到到高位地址
>
> ​        网络字节序为：大端序
>
> ​		如果某一主机以大端序向某小端序主机发送数据，次数小端序若不进行字节序转换将收到错误的数据。为了解决上述问题，在通过网络传输数据时约定了统一方式，这种约定称为**网络字节序**（Network Byte Order）,非常简单，统一为大端序。



（9）大端序计算机希望把4字节整数型数据12传递到小端序计算机。请说出数据传输过程中发生的字节序转换过程。

> 答：网络字节序为大端序，发送方无需转换字节序。接收方为小端序，需要把大端序0x12转换为小端序0x21

（10）怎样表示回送地址？其含义是什么？如果向回送地址传输数据将会发生什么情况？

> 回送地址为：127.0.0.1，指代本机IP。无论什么程序，一旦使用回送地址发送数据，协议软件立即返回之，不进行任何网络传输。

