# 第5章 基于TCP的服务器端/客户端（2）

## 1、回声客户端存在的问题

观察以下客户端代码：

```c
write(sock, message, strlen(message));
str_len = read(sock, message, BUF_SIZE-1);
message[str_len] = 0;
printf("Message from server: %s",message);
```

以上待码有个错误的假设：每次调用read，write函数时都会以字符串为单位执行实际的I/O操作。

**实际上**：TCP不存在数据边界，多次调用write函数传递的字符串有可能一次性传输到服务的，也有可能一次从服务端接收多个字符串。

问题不在服务端，而在客户端。从上述代码可以知道，客户端传输的是字符串，而且是通过调用write函数一次性发送的。之后又再一次调用read函数，**期待着接收自己传输的字符串**。这就是问题所在。

  

## 2、解决方法——确定接收数据的大小

前面回声客户端存在的问题是，无法确定接收数据的大小，可能是一个字符串，也可能是多个字符串。如果发送了一个20字节的字符串数据，在接收时循环调用read函数读取20个字节即可。

```c
int str_len = write(sock,message, strlen(message));    // str_len记录发送的字符串占用的字节数，意味着待会要接收str_len字节的数据
int recv_cnt = 0;  // 记录每次调用read函数读取的字节数
int recv_len = 0;  // 记录总共读取的字节数
while(recv_len<recv_cnt){   //当读取字节数等于发送字节数时退出循环
  recv_cnt = read(sock,&message[recv_len], BUF_SIZE-1);
  if(recv_cnt==-1){
    error_handling("read() error");
  }
  recv_len += recv_cnt;
  message[recv_len]=0;  //将message下标为recv_len的位置用字符串结束符'\0'取代
  printf("Message from server: %s",message);
}

```

> recv_cnt = read(sock,&message[recv_len], BUF_SIZE-1); 
>
> 我刚开始有一个疑问：会不会一次接收的数据就已经大于发送的数据了？
>
> 其实是不可能的。因为发送指定的字节数后，紧接着就通过循环读取了这部分数据。数据并不会在服务器端“囤积”



## 3、定义应用层协议

上述解决方案的提前是知道接收的数据长度。但大多数情况下这是不太可能的，真实网络环境中，我们往往并不能提前确认接收的数据长度。这时为了有效收发数据，我们就需要定义应用层协议。这就是==应用层协议的作用：应对应用层复杂的数据交互逻辑==。



### 3.1 示例

下面是一个实际问题的例子，通过写程序来实现一个简单的应用层协议。

**问题：假设终端中有以下执行过程**

```
Connect......
Operand count: 2            # 终端输入数字2 表示接下来要输入两个数字
Operand 1: 24               # 终端输入的第1个数字 24
Operand 2: 12								# 终端输入的第2个数字 12
operator: -                 # 终端输入运算符 -     
Operation result: 12        # 服务器返回结果12      其计算过程是：24-12    该过程在服务器端完成   
```

运算符可以是+  -  *

请编写程序实现上述功能，服务器端充当一个简单的计算器。



### 3.2 定义协议

* 客户端连接到服务器端后以1字节整数形式传递待算数字个数。
* 客户端向服务器端传递的每个整数型数据占用4字节。
* 传递整数型数据后紧接着传递运算符。运算符消息占用1字节
* 选择字符+、-、*之一传递
* 服务器端以4字节整数型向客户端传送运算结果
* 客户端得到运算结果后终止与服务器端的连接

<img src="https://wangjunblogs.oss-cn-beijing.aliyuncs.com/TCP-IP-Network-ch04/%E5%AE%A2%E6%88%B7%E7%AB%AFop_client%E7%9A%84%E6%95%B0%E6%8D%AE%E4%BC%A0%E9%80%81%E6%A0%BC%E5%BC%8F.png" alt="客户端op_client的数据传送格式" style="zoom: 50%;" />

### 3.3 代码实现

op_client.c

```c
#include <stdio.h>
#include <stdlib.h>     // 定义数值与字符串转换函数
#include <unistd.h>     // Unix Standard的缩写，用于提供对 POSIX 操作系统 API 的访问功能的头文件
#include <string.h>     // 含了宏定义、常量以及函数和类型的声明，还包括大量内存处理函数
#include <arpa/inet.h>  // 声明了字节序转换函数及地址结构体定义
#include <sys/socket.h> // 定义了socket相关函数和结构体定义

#define BUF_SIZE 1024   //从套接字接受数据的缓冲区
#define RLT_SIZE 4      //定义数字个数的字节数和运算结果的字节数
#define OPSZ 4

void error_handling(char *message);

int main(int argc,char* argv[])
{
    int sock;
    char opmsg[BUF_SIZE];
    int result, opnd_cnt;          //保存计算结果，待操作数个数
    struct sockaddr_in serv_adr;   //用于保存地址和端口号等信息
    if(argc!=3){                   //若参数不等于3，则退出程序
        printf("Usage:%s IP PORT\n",argv[0]); 
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock==-1)
        error_handling("socket() error");
    
    memset(&serv_adr,0,sizeof(serv_adr));        //将serv_adr结构体置为0
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr=inet_addr(argv[1]); //该函数将字符串转换为大端序整数型置，失败时返回INADDR_NONE
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("connect() error");
    else
        puts("Connected......"); //把一个字符串写入到标准输出 stdout，直到空字符，但不包括空字符。换行符会被追加到输出中。

    fputs("Operand count: ",stdout);
    scanf("%d",&opnd_cnt);
    opmsg[0] = (char)opnd_cnt;

    for(int i=0; i<opnd_cnt; ++i){
        printf("Operand %d\n", i+1);
        scanf("%d: ", (int *)&opmsg[i * OPSZ + 1]);
        //将地址为opmsg[i*OPSZ+1]char类型指针强制转换为int型指针
        //例如
        //当读取第一个数字时，opmsg[1] opmsg[2] opmsg[3] opmsg[4] 这四个字节将用于存储输入的int型整数
        //读取第二个数字时 opmsg[5] opmsg[6] opmsg[7] opmsg[8]这四个字节存储输入的第二个int型整数
        //服务器怎么得到opmsg中的数字？
        //很简单，服务器把opmsg数组，强转为int型数组，就可以读取其中的数字
        //原因：不管是char,还是int，本质上存的都是一系列二进制位，只要以存时的类型来读，就可以读出写入前的值
        //例如： 以int型写入，以int型读取

        }
        fgetc(stdin);  //吸收输入最后一个数字时产生的换行符'\n'
        fputs("Operator: ", stdout);

        scanf("%c: ", &opmsg[opnd_cnt*OPSZ+1]);
        write(sock,opmsg, opnd_cnt*OPSZ+2);
        read(sock, &result, RLT_SIZE);   //读取结果

        printf("Operation result: %d\n",result);
        close(sock);
        return 0;
}

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}
```



op_serve.c

```c
#include <stdio.h>
#include <stdlib.h>     // 定义数值与字符串转换函数
#include <unistd.h>     // Unix Standard的缩写，用于提供对 POSIX 操作系统 API 的访问功能的头文件
#include <string.h>     // 含了宏定义、常量以及函数和类型的声明，还包括大量内存处理函数
#include <arpa/inet.h>  // 声明了字节序转换函数及地址结构体定义
#include <sys/socket.h> // 定义了socket相关函数和结构体定义

#define BUF_SIZE 1024
#define OP_SIZE 4          //结果和数字个数所占用字节数

int calculate(int opnum, int opnds[], char op);
void error_handling(char *message);

int main(int argc, char *argv[])
{
    int serv_sock,clnt_sock;    //存储服务器套接字，和客户端套接字
    char opinfo[BUF_SIZE];
    char s[BUF_SIZE];
    int result,opnd_cnt;        //保存计算结果 数字个数
    int recv_cnt,recv_len;
    struct sockaddr_in serv_adr, clnt_adr;   //保存服务器端地址信息，客户端地址信息
    socklen_t clnt_adr_sz;   //保存客户端地址信息结构体长度

    if(argc!=2){
        printf("Usage: %s PORT",argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM,0);
    if(serv_sock==-1)
        error_handling("socket() error");
    
    memset(&serv_adr,0,sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
        error_handling("bind() error");
    
    if(listen(serv_sock,5)==-1)
        error_handling("listen() error");
    
    clnt_adr_sz = sizeof(clnt_adr);
    for(int i=0; i<5; ++i){
        opnd_cnt = 0;
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);
        read(clnt_sock,&opnd_cnt,1);

        recv_len=0;
        while((opnd_cnt*OP_SIZE+1)>recv_len){
            recv_cnt = read(clnt_sock, &opinfo[recv_len],BUF_SIZE-1);
            recv_len += recv_cnt;
        }

        result = calculate(opnd_cnt,(int*)opinfo, opinfo[recv_len-1]);
        write(clnt_sock,(char*)&result, sizeof(result));
        close(clnt_sock);
    }
    close(serv_sock);
    return 0;
}

int calculate(int opnum, int opnds[], char op)
{
    int result = opnds[0];
    switch(op){
        case '+':
            for(int i=1; i<opnum; ++i) result+=opnds[i];
            break;
        case '-':
            for(int i=1; i<opnum; ++i) result-=opnds[i];
            break;
        case '*':
            for(int i=1; i<opnum; ++i) result*=opnds[i];
            break;
    }
    return result;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}


```

### 3.4 运行结果

```shell
mygit@ubuntu:~/TCP-IP-Network-Note/ch05$ gcc op_server.c -o op_server  # 编译服务器端
mygit@ubuntu:~/TCP-IP-Network-Note/ch05$ ./op_server 9190  						 # 运行服务器端并绑定9190端口 
                                     																	 # 此时端口进入阻塞状态，等待客户端连接
```

用另一个终端或服务器端后台运行

```shell
mygit@ubuntu:~/TCP-IP-Network-Note/ch05$ gcc op_client.c -o op_client # 编译客户端
mygit@ubuntu:~/TCP-IP-Network-Note/ch05$ ./op_client 127.0.0.1 9190   # 连接IP为127.0.0.1(即本机)的9190端口
Connected......
Operand count: 4
Operand 1
2
Operand 2
4
Operand 3
6
Operand 4
1
Operator: +
Operation result: 13																									#正确返回了计算结果：2+4+6+1
```



## 4、TCP原理1: 与对方套接字建立连接

### 4.1 TCP套接字中的I/O缓冲

TCP套接字的数据收发无边界，服务器端可以一次性调用write函数传输40字节的数据，客户端也可能通过4次read函数调用每次读取10字节。这里有一个疑问，服务器端向客户端发送了40字节数据，**客户端接收了10字节后，另外30字节在哪？**

其实是在socket缓冲区里。**每个套接字都有输入缓冲和输出缓冲**，调用write函数，实际将数据写入输出缓冲区，调用read函数，实际是从输入缓冲区读取数据。



<img src="https://wangjunblogs.oss-cn-beijing.aliyuncs.com/TCP-IP-Network-ch04/tcp%E5%A5%97%E6%8E%A5%E5%AD%97%E7%BC%93%E5%86%B2.png" alt="tcp套接字缓冲" style="zoom: 50%;" />



**TCP套接字I/O缓冲特性**

* I/O缓冲在每个TCP套接字中单独存在。
* I/O缓冲在创建套接字时自动生成。
* 即使关闭套接字也会继续传输出缓冲中遗留的数据
* 关闭套接字将丢失输入缓冲中的数据

### 4.2 TCP滑动窗口协议

该协议==确保不会发生超过输入缓冲大小的数据传输==。

用对话方式来解释滑动窗口协议。假设套接字B向套接字A发送数据

> 套接字A：“你好，最多可以向我传递50字节。”
>
> 套接字B：“OK!”
>
> 套接字A：“我腾出了20字节空间，最多可以收70字节。”
>
> 套接字B：“OK!”



### 4.3 建立连接——三次握手

TCP是可靠的数据传输，那怎么实现可靠的数据传输？先从建立连接说起。

为了实现可靠的数据传输，在通信之前，通信双方都得知道对方有正常收发数据能力，这是实现可靠数据传输的基础。

通过三次握手，通信双方都知道对方能正常收发信息，从而建立建立。其具体过程如下：

> 第一次握手：
>
> 套接字A：“你好，套接字B，我这有数据要传给你，建立连接吧。”
>
> 第二次握手：
>
> 套接字B：“好的，我这边已就绪。”
>
> 第三次握手：
>
> 套接字A：”谢谢你受理我的请求。“



通过第一次握手，让B知道了==A有发送数据能力==。

通过第二次握手，让A知道==B有接收数据能力和发送数据能力==

这时B还不知道A是否有接收数据能力，所以还需要第三次握手

通过第三次握手，让B知道==A有接收数据能力==



**下图表示TCP三次握手过程**



<img src="https://wangjunblogs.oss-cn-beijing.aliyuncs.com/TCP-IP-Network-ch04/%E6%88%AA%E5%B1%8F2021-04-21%20%E4%B8%8B%E5%8D%884.03.09.png" alt="截屏2021-04-21 下午4.03.09" style="zoom:33%;" />



1. [SYN] SEQ:1000, ACK: - 

   这是首次请求连接时使用的消息，又称SYN，表示==收发数据前的同步消息==。

   > SEQ:1000 表示当前传递的数据包序号为1000，如果接收无误，请通知我向您传递1001号数据包。



2. [SYN+ACK] SEQ: 2000，ACK: 1001

   对主机A首次传输的数据包的确认消息（ACK1001）和为主机B传输消息做准备的同步消息（SEQ2000）捆绑发送，因此，此种类型的消息又称==SYN+ACK==。

   > SEQ为2000的含义：表示现传递的数据包序号为2000，如果接收无误，请通知我向您传递2001号数据包
   >
   > ACK 1001的含义：表示主机B已经成功接收1000号数据包。



3. [ACK] SEQ:1001，ACK: 2001

   > ACK：已正确收到SEQ为2000的数据包，现在可以传输序号为1001的数据包。

   **至此，主机A和主机B俊确认了彼此均就绪**

==收发数据前向数据包分配序号，并向对方通报此序号，这是为了防止数据丢失所做的准备。通过向数据包分配序号并确认，可以在数据丢失时马上查看并重传丢失的数据。因此，TCP可以保证可靠的数据传输==



## 5、TCP原理2: 与对方主机的数据交换

### 5.1 数据交换未发生错误

通过第一步三次握手过程完成了数据交换准备。下面就正式开始收发数据，其默认方式如图所示：

<img src="https://wangjunblogs.oss-cn-beijing.aliyuncs.com/TCP-IP-Network-ch04/tcp%E5%A5%97%E6%8E%A5%E5%AD%97%E7%9A%84%E6%95%B0%E6%8D%AE%E4%BC%A0%E8%BE%93%E8%BF%87%E7%A8%8B.png" alt="tcp套接字的数据传输过程" style="zoom:33%;" />

第一次主机A向主机B发送了100字节的数据，并把第一个字节编号为1200。主机B为了告诉主机A已成功接收发送的100字节数据，向主机A发送ACK1301，意思是，编号到1300，包括1300的数据已成功接收，请发送开始编号为1301的数据。



第二次主机A向主机B又发送了100字节的数据，并把第一个字节标记为1301。同样，主机B向主机A发送ACK1402，表示已经成功接收编号到1401，包括1401的数据。



==接收方成功接收数据后，向发送方发送的确认消息，ACK=SEQ号 + 传递的字节数 + 1==

 

### 5.2 数据交换发生了丢失

当发送的数据丢失时，TCP通过以下方式解决。

<img src="https://wangjunblogs.oss-cn-beijing.aliyuncs.com/TCP-IP-Network-ch04/tcp%E5%A5%97%E6%8E%A5%E5%AD%97%E6%95%B0%E6%8D%AE%E4%BC%A0%E8%BE%93%E5%8F%91%E7%94%9F%E9%94%99%E8%AF%AF.png" alt="tcp套接字数据传输发生错误" style="zoom:67%;" />

如上图所示，当主机A向主机B发送SEQ为1301的数据包时发生了丢失，在规定的时间内主机B没回复ACK，因此主机A认为数据已经丢失，然后再次向主机B重传相同的数据，直到主机B发送了ACK确认消息。



### 5.3 数据交换发生数据错误

假如传输了100个数据包，第99个发生了错误怎么办？

解决方法也很简单，回复发送方我只接收到了前98个数据包。即回复ACK 99，表示我只接收到了前98个数据包，下次给我传输编号为99开始的数据包。



## 6、TCP原理3: 断开与套接字的连接

TCP套接字的结束也非常优雅。如果对方还有数据没有传输完就直接断掉就会出现问题，所以断开连接前需要协商。

断开套接字时，双发进行的对话：

> 套接字A：“我希望断开连接。”
>
> 套接字B：“哦，是吗？请稍后。”
>
> 套接字B：“我也准备就绪，可以断开连接。”
>
> 套接字A：“好的，谢谢合作。”

先由套接字A向套接字B传递断开连接的消息，套接字B发出确认收到的消息，过一会套接字B向套接字A发送可以断开消息，套接字A又回复确认收到消息。这时双方都可以关闭了。

如下图所示：

<img src="https://wangjunblogs.oss-cn-beijing.aliyuncs.com/TCP-IP-Network-ch04/tcp%E5%A5%97%E6%8E%A5%E5%AD%97%E6%96%AD%E5%BC%80%E8%BF%87%E7%A8%8B.png" alt="tcp套接字断开过程" style="zoom:50%;" />



双方各发送一次FIN消息以后断开连接，次过程经历4个阶段，又称为**4次握手**。

> 上图中，主机B向主机A发送了两次ACK 5001，是否让各位刚到困惑？
>
> 其实第一次向主机ACK 5001是为了确认已经收到主机A发送的FIN。第二次是上次发送了ACK后未接收数据而重传的数据包，只是把SEQ置为7501，表示现发送7501，若成功收到则回复ACK7502



## 7、习题

（1）请说明TCP套接字连接设置的三次握手过程。尤其是3次数据交换过程每次收发的数据内容。

> 答：假设套接字A请求连接套接字B。
>
> 第一次握手，套接字A向套接字B发送SYN消息，SEQ值为1000，ACK为空，即告诉套接字B现在发送的数据包SEQ为1000，收到请回复ACK1001
>
> 结果：套接字B知道套接字A有发送能力
>
> 第二次握手，套接字B向套接字A回复SYN+ACK消息，SEQ值为2000，ACK值为1001，即告诉套接字A现在给你发送数据包SEQ为2000，收到请回复ACK2001
>
> 结果：套接字A知道套接字B有接收能力和发送能力
>
> 第三次握手，套接字A向套接字B回复ACK消息，SEQ为1001，ACK为2001，告诉套接字B，已正确收到SEQ为2000的数据包，现在可以传输SEQ为2001的数据包
>
> 结果：套接字B知道套接字A有接收能力。



（2）TCP是可靠的数据传输协议，但在通过网络通信的过程中可能丢失数据。请通过ACK和SEQ说明TCP通过何种机制保证丢失数据的可靠传输。

> 答：通过超时重传机制，每次发送数据都会启动一个定时器，若在一定时间内对方没有回复ACK，则表示上次传输的数据包已经丢失，这时就需要重传数据包。
>
> 例如A向B传输100字节的数据，SEQ为1200，若在规定时间内没有收到ACK为1301的数据包，则表示数据包已经丢失，这时A会重传数据。

（3）TCP套接字中调用write函数和read函数时数据如何移动？结合I/O缓冲进行说明。

> 答：套接字调用write函数后，并不立即发送数据，而是先进入输出缓冲，然后在适当的时候发送到对方的输入缓冲。
>
> 套接字调用read函数后，并不马上接收数据，而是从输入缓冲读取数据。

（4）对方主机的输入缓冲剩余50字节空间时，若本方主机通过write函数请求传输70字节，请问TCP如何处理这种情况？

> 答：对方TCP通过滑动窗口协议告诉本方主机，只能接受50字节数据，这时只能向对方传输50字节数据，对方接受后腾出空间，再传输剩余的20字节。



（5）第2章示例tcp_server.c（第1章的hello_server.c）和tcp_client.c中，客户端接收服务器端传输的字符串后便退出。现更改程序，使服务器和客户端各传递1次字符串。考虑到使用TCP协议，所以传递字符串前先以4字节整数型方式传递字符串长度。连接时服务器端和客户端数据传输格式如下。

<img src="https://wangjunblogs.oss-cn-beijing.aliyuncs.com/TCP-IP-Network-ch04/%E6%88%AA%E5%B1%8F2021-04-21%20%E4%B8%8B%E5%8D%8811.46.01.png" alt="截屏2021-04-21 下午11.46.01" style="zoom: 67%;" />



另外，不限制字符串传输顺序和种类，但需进行3次数据交换。

(6) 创建收发文件的服务器端/客户端，实现顺序如下。


(5)和(6)的代码不幸遗失，以后有机会加上








