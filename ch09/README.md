# 第9章 套接字的多种可选项

## 1、套接字可选项和I/O缓冲大小

### 1.1 套接字的可选项

通过套接字的可选项，可以控制更多套接字的底层工作信息

![套接字可选类型](https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E5%A5%97%E6%8E%A5%E5%AD%97%E5%8F%AF%E9%80%89%E7%B1%BB%E5%9E%8B.png)

![套接字可选类型2](/Users/wangjun/computer-system/计算机网络原理/TCP-IP-网络编程/ch09/套接字可选类型2.png)



### 1.2 getsockopt()

该函数用于获取套接字的可选项信息

```c
#include <sys/socket.h>

int getsockopt(int sock, int level, int optname, void *optval, socklen_t *optlen);

//成功时返回0，失败时返回-1
/*
	sock 套接字文件描述符
	level 要查看的可选项的协议层
	optname 要查看的可选项名
	optval 保存查看结果的缓冲地址值
	optlen 向第4个参数optval传递的缓冲大小
*/
```

#### 实例：获取SO_TYPR

查看TCP套接字和UDP套接字的SO_TYPE信息

```c
int sock_type;
socklen_t optlen;
optlen = sizeof(sock_type);

getsockopt(tcp_sock, SOL_SOCKET, SO_TYPE, (void*)&sock_type, &optlen);
getsockopt(udp_sock, SOL_SOCKET, SO_TYPE, (void*)&sock_type, &optlen);
//tcp_sock  tcp套接字
```

SO_TYPE是典型的只读可选项，只能在创建时决定，以后不能更改。



### 1.3 setsockopt()

该函数用于设置套接字的可选项信息

```c
#include <sys/socket.h>

int setsockopt(int sock, int level, int optname, void *optval, socklen_t *optlen);
// 成功时返回0 失败时返回-1
/*
	sock 套接字文件描述符
	level 要查看的可选项的协议层
	optname 要查看的可选项名
	optval 保存查看结果的缓冲地址值
	optlen 向第4个参数optval传递的缓冲大小
*/
```



### 1.4 SO_SNDBUF & SO_RCVBUF

SO_SNDBUF表示输出缓冲区大小可选项

SO_RCVBUF表示输入缓冲区大小可选项



#### 实例：获取socket缓冲区大小

```c
int snd_buf, rcv_buf;
socklen_t optlen;

len = sizeof(snd_buf);
getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (void*)&snd_buf, &len);
len = sizeof(rcv_buf);
getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void*)&rcv_buf, &len);
```

结果可能因系统而异。



#### 实例：更改socket缓冲区大小

```c
int snd_buf = 1024*3, rcv_buf=1024*3;
socklen_t optlen;
optlen = sizeof(sock_type);

setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (void*)&snd_buf, sizeof(snd_buf));
setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void*)&rcv_buf, sizeof(rcv_buf));
```



## 2、SO_REUSEADDR

### 2.1 发生Binding Error

参照之前的回声客户端，当我们在客户端通过CTRL-C终止程序时，我们还可以重新连接服务器，似乎没有任何影响。

但当我们在服务器端通过CTRL-C终止程序时，再启动服务器端，就会发生Binding Error错误。为什么？因此此时端口号还未释放，因而无法再分配相同的端口号，但过了几分钟后，发现这个端口号又可以用了，至于为什么？先了解Time-wait状态



### 2.2 Time-wait状态

当服务器或客户端中的任何一方通过CTRL-C强制退出，或系统执行close函数退出程序，那么该套接字会进入 Time-wait状态，如下图所示。

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/Time-wait%E7%8A%B6%E6%80%81%E4%B8%8B%E7%9A%84%E5%A5%97%E6%8E%A5%E5%AD%97.png" alt="Time-wait状态下的套接字" style="zoom:33%;" />



#### Time-wait出现原因

当套接字断开连接时，不管是CTRL-C(由操作系统关闭文件及套接字)或正常退出，会经过四次挥手，如图9-1，当主机A向主机B发送最后一次ACK数据包时立即断开连接，若之后该数据包丢失，主机B会以为之前传递的FIN消息(SEQ 7501，ACK 5001)未能抵达主机A，继而试图重传。但主机A已是完全终止的状态，因此主机B永远也无法收到主机A组后传来的ACK消息。相反，如果主机A的套接字处在Time-wait状态，则会向主机B重传最后的ACK消息，主机B也可以正常终止。

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-04-30%20%E4%B8%8A%E5%8D%8811.23.06.png" alt="截屏2021-04-30 上午11.23.06" style="zoom: 33%;" />



客户端套接字端口号因为是随机分配。所以当再次连接服务器时，无法感受到Time-wait的存在。

当服务器端再次分配相同的端口号时，就会出现Binding Error错误，因为此时该端口号处于Time-wait状态，大概持续几分钟时间。





### 2.3 地址再分配

Time-wait看似重要，但不一定讨人系统，因为当系统出现故障，需要紧急重启时，但因处于Time-wait状态而必须等待几分钟，如果网络状态不理想，Time-wait状态还将持续。

解决方案就是在套接字可选项中更改SO_REUSEADDR的状态，适当调整该参数，可将Time-wait状态下的套接字端口号重新分配给新的套接字。SO_REUSEADDR的默认值为0，改为1就可以让将端口号分配给新的套接字。

```c
socklen_t optlen;
int option;
optlen = sizeof(option);
option = TURE;
setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&option, optlen);
```



## 3、TCP_NODELAY

### 3.1 Nagle 算法

为防止因数据包过多而发生网络过载，Nagle算法在1984年诞生，它应用于TCP层，其使用与否会导致下列差异：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/Nagle%E7%AE%97%E6%B3%95.png" alt="Nagle算法" style="zoom:50%;" />

Nagle 算法具体内容：只有收到前一数据的ACK消息时，才发送下一数据。



Nagle算法并不是什么时候都适用。根据传输数据的特性，网络流量未受太大影响时，不使用Nagle算法要比使用它时传输速度更快。最典型的就是"传输大文件"时。将文件数据传输输出缓冲不会花太多时间，因此，即便不使用Nagle算法，也会在装满输出缓冲时传输数据包。这不仅不会增加数据包的数量，反而会在无需等待ACK的前提下连续传输，因而可以大大提高传输速度。



### 3.2 禁用Nagle算法

只需将TCP_NODELAY改为1

```c
int opt_val = 1;
setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void*)&opt_val, sizeof(opt_val));
```



也可以查看TCP_NODELAY值的设置状态

```c
int opt_val;
socklen_t opt_len;
opt_len = sizeof(opt_val);
getsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void*)&opt_val, &optlen);
```

opt_val为0表示正在使用使用Nagle算法，为1，表示已经禁用Nagle算法



## 4、习题（参考答案）

4.1 下列关于Time-wait状态的说法错误的是？

1. Time-wait 状态只在服务器的套接字中发生
2. 断开连接的四次握手过程中，先传输 FIN 消息的套接字将进入 Time-wait 状态。
3. Time-wait 状态与断开连接的过程无关，而与请求连接过程中 SYN 消息的传输顺序有关
4. Time-wait 状态通常并非必要，应尽可能通过更改套接字可选项来防止其发生

> 答：1，3，4



4.2 TCP_NODELAY 可选项与 Nagle 算法有关，可通过它禁用 Nagle 算法。请问何时应考虑禁用 Nagle 算法？结合收发数据的特性给出说明。

> 答：当传输大文件时，若网络流量未受太大影响时，可以禁用 Nagle 算法。因为这样可以在没有收到ACK消息的情况下连续发送数据包，可以大大提高传输速度。



