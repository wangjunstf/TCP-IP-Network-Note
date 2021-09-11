# 第6章 基于UDP的服务器端/客户端

## 一、UDP概述

### 1.1 TCP和UDP的区别

1. UDP位于4层TCP/IP协议栈中，从上往下数第二层，与TCP协议处于相同的位置。

2. ==TCP是可靠的网络通信协议==，每次通信都要经过3次握手建立连接，中间的数据传输也会通过ACK确认，SEQ给数据包编号，有确认、窗口、重传、拥塞控制机制，来确保数据的安全传输。最后断开连接也需要经过四次挥手来安全断开连接。可以看到，即使发送一条数据量很小的信息，也需要经过多次额外的数据包来确认信息的传输，==传输效率低==。

3. ==UDP是不可靠的网络通信协议==，相比TCP，UDP则不需要额外的数据包来确认，而是直接将数据包发送给对方。因而==数据传输效率要高于TCP==。

4. ==TCP区分服务器端和客户端，UDP不区分服务器端和客户端==。
5. TCP首部开销20字节，UDP首部开销8字节。
6. 大多数情况，TCP的传输速度无法超过UDP，但也存在特殊情况：每次传输的数据量越大，TCP的传输速率就越接近UDP的传输速率。

### 1.2 TCP的优缺点

**TCP的优点**：安全可靠稳定

**TCP的缺点**：由于需要对数据包进行额外的确认，因而传输效率较低，占用系统资源高，易被攻击。

> 一台计算机能与其他计算机建立的TCP连接是有限的，当有恶意攻击者占用了全部TCP连接，合法用户就无法再与服务器建立连接。这就是拒绝服务攻击。例如DDOS，CC等攻击。

**应用场景**：

* 数据量较小且需要百分百数据不丢失的数据传输，例如压缩包的传输，用户的交易信息等。
* 对数据要求准确无误传输，例如HTTP，HTTPS，FTP，远程连接等
* 基于邮件的POP，SMTP等

### 1.3 UDP的优缺点

**UDP优点**：传输效率高，例如网络实时传输视频或音频。

**UDP缺点**：不可靠的数据传输，易发生数据丢失或错误。

**应用场景**：

* 适用于数据量较大，对传输效率要求较高，允许少量数据丢失或损坏的数据传输
* 例如网络直播，语音通话等



## 二、UDP的工作原理

### 2.1 UDP的工作原理

UDP是通过数据包的形式发送到目标主机，对UDP而言，每次只发送一个数据包，每个数据包都是独立的一条数据，数据包与数据包之间没有直接关联。

每台主机可通过一个套接字给多个主机发送数据，也可以由一个套接字接收多个主机发送的数据。

### 2.2 执行过程

1. 创建套接字

   ```c
   int serv_sock =  socket(PF_INET, SOCK_DGRAM, 0); 
   //成功时返回文件描述符，失败时返回-1
   /*
   	PF_INET 指IPv4
   	SOCK_DGRAM UDP协议
   */
   ```

2. 给套接字绑定地址信息

   ```c
   bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))
   //成功时返回0，失败时返回-1
   /*
   	serv_sock 发送端或接收到文件描述符，serv_adr本机地址信息结构体变量名
   */
   ```

3. 发送数据

   ```c
   #include <sys/socket.h>
   
   ssize_t sendto(int sock, void *buf, size_t nbytes, int flags, struct sockaddr *to, socklen_t addrlen);
   //成功时返回传输的字节数，失败时返回-1
   
   /*
   	sock 传输数据的UDP套接字文件描述符
   	buf 保存待传数据的缓冲区地址
   	nbytes 传输的字节数
   	flags 可选项参数，若无则传0
   	to 目标地址信息的结构体变量的地址值
   	addrlen to的变量大小
   */
   ```

4. 接收数据

   ```c
   #include <sys/socket.h>
   
   ssize_t recvfrom(int sock, void *buf, size_t nbytes, int flags, struct sockaddr *from, socklen_t addrlen);
   //成功时返回接收的字节数，失败时返回-1
   
   /*
   	sock 传输数据的UDP套接字文件描述符
   	buf 保存接收数据的缓冲区地址
   	nbytes 传输的字节数
   	flags 可选项参数，若无则传0
   	from 发送端信息的结构体变量的地址值
   	addrlen to的变量大小
   */
   
   ```

### 2.3 基于UDP的回声服务器端/客户端代码示例

描述：编写服务器端程序和客户端程序，客户端向服务器端发送一条信息，服务器端将信息原路返回，就像回声一样。

udp_echo_server.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 20
void error_handling(char* message);

int main(int argc, char* argv[]){
    int serv_sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t clnt_sock_len;

    struct sockaddr_in serv_adr, cln_adr;

    if(argc!=2){
        printf("Usage:%s <port>\n",argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_DGRAM,0);
    if(serv_sock == -1){
        error_handling("UDP creation is error");
    }

    memset(&serv_adr,0,sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1){
        error_handling("bind() error");
    }

    while(1){
        clnt_sock_len = sizeof(cln_adr);
        str_len = recvfrom(serv_sock, message, BUF_SIZE,0,(struct sockaddr*)&serv_adr, &clnt_sock_len);
        sendto(serv_sock, message,str_len,0,(struct sockaddr*)&serv_adr, clnt_sock_len);
    }

    close(serv_sock);

    return 0;
}

void error_handling(char* message){
    fputs(message, stderr);
    fputc('\n',stderr);
    exit(1);
}

```



udp_echo_client.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30

void error_handling(char *message);

int main(int argc, char* argv[]){
    int sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t adr_len;

    struct sockaddr_in serv_adr, from_adr;

    if(argc!=3){
        printf("Usage: %s <IP> <PORT>\n",argv[1]);
        exit(1);
    }

    sock = socket(AF_INET, SOCK_DGRAM,0);
    if(sock==-1){
        error_handling("socket() error");
    }

    memset(&serv_adr, 0 , sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[2]));

    while(1){
        fputs("Insert message(q to quit)",stdout);
        fgets(message,sizeof(message),stdin);

        if(!strcmp("q\n",message)||!strcmp("Q!\n",message)){
            break;
        }

        sendto(sock, message, strlen(message),0, (struct sockaddr*)&serv_adr, sizeof(serv_adr));

        adr_len = sizeof(from_adr);
        str_len = recvfrom(sock, message, BUF_SIZE, 0, (struct sockaddr*)&from_adr, &adr_len);

        message[str_len] = 0;
        printf("Message from server:%s",message);
    }

    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

```



### 2.4 编译并执行

```shell
$ gcc udp_echo_server.c /bin/udp_echo_server
$ ./bin/udp_echo_server 9190

$ gcc udp_echo_client.c /bin/udp_echo_client
$ ./bin/udp_echo_client 127.0.0.1 9190
Insert message(q to quit)hello
Message from server:hello
Insert message(q to quit)hello wold
Message from server:hello wold
Insert message(q to quit)
```



## 三、高性能UDP

### 3.1 UDP套接字地址分配

观察udp_echo_client.c，他缺少套接字地址的分配过程。TCP客户端调用connect函数自动将IP地址和端口号分配给套接字。而**UDP则是在发送数据的时候给套接字临时绑定IP地址和端口号**，数据传输完成就解除绑定。这样的特性，可以让主机用同一个套接字给不同的主机发送数据。同样，在**UDP在接收数据的时候给套接字临时绑定IP地址和端口号**，这样就可以从不同的主机接收数据。



### 3.2 已连接套接字与未连接套接字

UDP可以不设置UDP套接字的地址信息，那么套接字在发送和接收数据的时候临时绑定地址信息，在需要与同一主机进行长时间通信时，这种设置显然不是最高效的选择。



我们可以用以下函数给UDP套接字分配地址信息：提前分配地址信息的套接字称为**已连接套接字**，反之则为**未连接套接字**。

```c
connect(sock, (struct sockaddr *)&adr, sizeof(adr));

//这样就可以调用write和read函数进行发送和接收数据。
```



### 3.3 已连接套接字代码示例

将udp_echo_client.c改为已连接套接字。

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30

void error_handling(char *message);

int main(int argc, char *argv[])
{
    int sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t adr_len;

    struct sockaddr_in serv_adr, from_adr;

    if (argc != 3)
    {
        printf("Usage: %s <IP> <PORT>\n", argv[1]);
        exit(1);
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
    {
        error_handling("socket() error");
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[2]));

    connect(sock, (struct sockaddr*)&serv_adr,sizeof(serv_adr));
    while (1)
    {
        fputs("Insert message(q to quit)", stdout);
        fgets(message, sizeof(message), stdin);

        if (!strcmp("q\n", message) || !strcmp("Q!\n", message))
        {
            break;
        }

        // sendto(sock, message, strlen(message), 0, (struct sockaddr *)&serv_adr, sizeof(serv_adr));
        write(sock,message,strlen(message));

        // adr_len = sizeof(from_adr);
        // str_len = recvfrom(sock, message, BUF_SIZE, 0, (struct sockaddr *)&from_adr, &adr_len);

        str_len = read(sock,message,BUF_SIZE-1);
        message[str_len]=0;

        printf("Message from server:%s", message);
    }
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



### 3.2 编译并运行

```shell
$ gcc udp_echo_server.c /bin/udp_echo_server
$ ./bin/udp_echo_server 9190
```

```shell
$ gcc u_con_echo_client.c -o ./bin/u_con_echo_client
$ mygit@ubuntu:~/computer-system/Linux/src$ ./bin/u_con_echo_client 127.0.0.1 9190
Insert message(q to quit)hello world
Message from server:hello world
Insert message(q to quit)how are you
Message from server:how are you
Insert message(q to quit)123123
Message from server:123123
Insert message(q to quit)
```



## 四、习题

（1）UDP为什么比TCP速度快？为什么TCP数据传输可靠而UDP数据传输不可靠？

> 答：UDP直接传送数据包，并不需要额外的数据来验证传输的的数据。而TCP发送的数据包，需要通过ACK来验证，用SEQ给数据编号，用确认、窗口、重传、拥塞控制机制，来确保数据的安全传输。所以UDP比TCP传输速度快，但UDP传输的数据并不可靠，可能有数据包丢失或损坏。
>
> TCP数据传输有多重保障所以更加可靠。
>
> UDP只管发送，网络通信的不可靠性决定了UDP的不可靠。

（2）下列不属于UDP特点的是：

1. UDP 不同于 TCP ，不存在连接概念，所以不像 TCP 那样只能进行一对一的数据传输。
2. 利用 UDP 传输数据时，如果有 2 个目标，则需要 2 个套接字。
3. UDP 套接字中无法使用已分配给 TCP 的同一端口号
4. UDP 套接字和 TCP 套接字可以共存。若需要，可以同时在同一主机进行 TCP 和 UDP 数据传输。
5. 针对 UDP 函数也可以调用 connect 函数，此时 UDP 套接字跟 TCP 套接字相同，也需要经过 3 次握手阶段

> 答：2，3，5不属于UDP特点
>
> 2（一个套接字可以给多个套接字发送数据）
>
> 3  UDP和TCP可以共用一个端口号，并不会冲突，数据接收时时根据五元组{传输协议，源IP，目的IP，源端口，目的端口}判断接受者的
>
> 5  调用 connect 函数只是为了绑定地址信息。
>
> 1，4属于UDP的特点

（3）UDP数据报向对方主机的UDP套接字传递过程中，IP和UDP分别负责哪些部分？

> 答：数据在物理链路中传输由IP协议负责，到的目标主机后，由UDP协议负责。

（4）UDP一般比TCP快，但根据交换数据的特点，其差异可大可小。请说明何种情况下UDP的性能优于TCP？

> 答：当交换的数据比较小时，UDP的性能优于TCP。

（5）客户端TCP套接字调用connect函数时自动分配IP和端口号。UDP中不调用bind函数，那何时分配IP地址和端口号？

> 答：在实际接收数据和发送数据时给套接字分配IP地址和端口号。

（6）TCP客户端必须调用connect函数，而UDP中可以选择性调用。请问，在UDP中调用connect函数有哪些好处？

> 答：可以省去以下两个步骤所产生的时间：
>
> 向UDP套接字注册IP和端口
>
> 删除UDP套接字中注册的目标地址信息

（7）请参考本章给出的uecho_server.c和echo_client.c，编写示例使服务器端和客户端轮流收发消息。收发的消息均要输出到控制台窗口。

print_client.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30

void error_handling(char *message);

int main(int argc, char *argv[])
{
    int sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t adr_len;

    struct sockaddr_in serv_adr, from_adr;

    if (argc != 3)
    {
        printf("Usage: %s <IP> <PORT>\n", argv[1]);
        exit(1);
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
    {
        error_handling("socket() error");
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[2]));

    connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr));
    while (1)
    {
        
        fputs("Insert message(q to quit)", stdout);
        fgets(message, sizeof(message), stdin);

        if (!strcmp("q\n", message) || !strcmp("Q!\n", message))
        {
            break;
        }

        // sendto(sock, message, strlen(message), 0, (struct sockaddr *)&serv_adr, sizeof(serv_adr));
        write(sock, message, strlen(message));

        // adr_len = sizeof(from_adr);
        // str_len = recvfrom(sock, message, BUF_SIZE, 0, (struct sockaddr *)&from_adr, &adr_len);

        str_len = read(sock, message, BUF_SIZE - 1);
        if (str_len != 0)
        {
            message[str_len] = 0;
            printf("Message from server:%s", message);
        }
    }
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



print_server.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 20
void error_handling(char *message);

int main(int argc, char *argv[])
{
    int serv_sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t clnt_sock_len;

    struct sockaddr_in serv_adr, cln_adr;

    if (argc != 2)
    {
        printf("Usage:%s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (serv_sock == -1)
    {
        error_handling("UDP creation is error");
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        error_handling("bind() error");
    }

    while (1)
    {
        clnt_sock_len = sizeof(cln_adr);
        str_len = recvfrom(serv_sock, message, BUF_SIZE, 0, (struct sockaddr *)&serv_adr, &clnt_sock_len);
        message[str_len]=0;
        if(str_len!=0){
            printf("Message from server:%s", message);
        }

        fputs("Insert message(q to quit)", stdout);
        fgets(message, sizeof(message), stdin);

        if (!strcmp("q\n", message) || !strcmp("Q!\n", message))
        {
            break;
        }

        sendto(serv_sock, message, str_len, 0, (struct sockaddr *)&serv_adr, clnt_sock_len);
    }

    close(serv_sock);

    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

```

