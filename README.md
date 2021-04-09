# TCP-IP-Network-Note
本仓库是我的《TCP/IP网络编程》学习笔记以及代码实现。笔者目前使用Linux平台，因此本笔记只包括Linux部分。如果本笔记对您有用，别忘了点一个star。转载请注明出处，谢谢。

**运行环境**
> 操作系统： Ubuntu 18.04.5 LTS
>
> gcc 8.4.0 (Ubuntu 8.4.0-1ubuntu1~18.04)
>
> g++ 8.4.0 (Ubuntu 8.4.0-1ubuntu1~18.04)

本项目的Github地址：[TCP-IP-Network-Note](https://github.com/wangjunstf/TCP-IP-Network-Note)。

如果在阅读本笔记的过程中发现`错别字`或者 `bug` ，欢迎向本项目提交 `PR`。



## 第1章 理解网络编程和套接字

### 1.1 理解网络编程和套接字

套接字是网络数据传输用的软件设备。

#### 1.1.1 以电话机的安装和使用过程来理解套接字

电话机可以同时用来拨打或接听，但对套接字而言，拨打和接听是有区别的。

**调用socket函数（安装电话机）时进行的对话**

> 问：“接电话需要准备什么？”
>
> 答：“当然是电话机！”

有了电话机才能安装电话，下面的函数相当于准备一部漂亮的电话机，即套接字。

```c
#include <sys/socket.h>
int socket(int domin, int type, int protocol); 
//成功时返回文件描述符，失败时返回-1
```

有了电话机，就要考虑分配电话号码的问题，这样别人才能与自己通信。



**调用bind函数（分配电话号码）时进行的对话**

> 问：“请问您的电话号码是多少？”
>
> 答：“我的电话号码是123-1234。”

电话号码相当于电话机的地址，利用以下函数给创建好的套接字分配地址信息（IP地址和端口号）

```c
#include <sys/socket.h>
int bind(int sockfd, struct sockaddr *myaddr, socklen_t addrlen);
//成功时返回0，失败时返回-1
```



**调用listen函数（连接电话线）时进行的对话**

> 问：“已架设完电话机后是否只需连接电话线？”
>
> 答：“对，只需连接就能接听电话。”

一连接电话线，电话机就转为可接听状态。同样的，下面的函数把套接字转化为可接听的状态。

```c
#include <sys/socket.h>
int listen(int sockfd, int backlog);
//成功时返回0，失败时返回-1
```



**调用accept函数（拿起话筒）时进行的对话**

> 问：“电话铃响了，我该怎么办？”
>
> 答：“难道您真不知道，接听啊！”

拿起话筒意味着接收了对方的连接请求。套接字也是如此，如果有人为了完成数据传输而请求连接，就需要调用以下函数进行受理。

```c
#include <sys/socket.h>
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
//成功时返回文件描述符，失败时返回-1
```



网络编程中接受连接请求的套接字创建过程可整理如下。

- 第一步：调用socket函数创建套接字。
- 第二步：调用bind函数分配IP地址和端口号。
- 第三步：调用listen函数转为可接收请求状态。
- 第四步：调用accept函数受理连接请求



#### 1.1.2 编写“Hello world！”套接字程序

**服务端**

服务器端（server）是能够受理连接请求的程序。下面构建服务器端以验证之前提到的函数调用过程，该服务器端收到连接请求后向请求者返回“Hello world!”答复。除各种函数的调用顺序外，我们还未涉及任何实际编程。因此，阅读代码请重点关注套接字相关函数的调用过程，不必理解全部示例。

服务端代码请参见：[hello_server.c](https://github.com/wangjunstf/TCP-IP-Network-Note/blob/main/ch01-%E7%90%86%E8%A7%A3%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B%E5%92%8C%E5%A5%97%E6%8E%A5%E5%AD%97/src/hello_server.c)



**客户端**

客户端程序只有“`调用socket函数创建套接字`”和“`调用connect函数向服务器端发送连接请求`”这两个步骤。下面给出客户端，查看以下两项内容：

1. 调用socket函数和connect函数
2. 与服务端共同运行以收发字符串数据

客户端代码请参见：[hello_client.c](https://github.com/wangjunstf/TCP-IP-Network-Note/blob/main/ch01-%E7%90%86%E8%A7%A3%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B%E5%92%8C%E5%A5%97%E6%8E%A5%E5%AD%97/src/hello_server.c)



**编译**

分别对客户端和服务端进行编译：

```shell
gcc hello_server.c -o hserver
gcc hello_client.c -o hclient
```



**运行**

运行服务器端程序，并指定端口号为9190：`./hserver 9190`

在另一个终端窗口中运行客户端程序，并指定ip地址和端口号：`./hclient 127.0.0.1 9190`

127.0.0.1代表本机ip

**单终端运行方法**

> 运行服务器端程序后，当前终端将进入阻塞塞，等待客户端的连接，对于只有一个终端窗口来说并不方便，可以将其在后台运行，然后运行客户端程序，也能实现同样的功能。
>
> ```shell
> ./hserver 9190 &  #符号&告诉shell，该程序在后台运行，shell还会返回该进程id，可以通过进程id结束该程序，例如 kill -s 9 进程id
> ./hclient 127.0.0.1 9190
> ```



### 1.2 基于Linux的文件操作

讨论套接字的过程中突然谈及文件也许有些奇怪。但是对于 Linux 而言，socket 操作与文件操作没有区别，因而有必要详细了解文件。Linux里提倡一切皆文件，socket也不例外。在 Linux 世界里，socket 也被认为是文件的一种，因此在网络数据传输过程中自然可以使用 I/O 的相关函数。Windows 与 Linux 不同，是要区分 socket 和文件的。因此在 Windows 中需要调用特殊的数据传输相关函数。

#### 1.2.1 底层文件访问和文件描述符

”底层“可以理解为“与标准无关的操作系统独立提供的”。稍后讲解的函数是由Linux提供，而非ANSI标准定义的函数。如果想使用Linux提供的文件I/O函数，首先应该理解好文件描述符的概念。

此处的文件描述符是系统分配给文件或套接字的整数。实际上，学习c语言过程中用过的标准输入输出及标准错误在Linux中也被分配表1.1中的文件描述符。

表1.1 分配给标准输入输出及标准错误的文件描述符

| 文件描述符 | 对象                     |
| :--------- | ------------------------ |
| 0          | 标准输入 Standard Input  |
| 1          | 标准输出 Standard Output |
| 2          | 标准错误 Standard Error  |

文件和套接字一般经过创建过程才会被分配文件描述符。

实际上，文件描述符只不过是为了方便称呼操作系统创建的文件或套接字而赋予的数字而已。

文件描述符有时也称为文件句柄，但“句柄”主要是windows中的术语。因此本书若涉及Windows平台将使用“句柄”，Linux平台则用"文件描述符"

#### 1.2.2 打开文件

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *path, int flag);
/*
	成功时返回文件描述符，失败时返回-1
	
	path 为打开的目标文件名及路径信息
	flag 为文件打开模式
*/
```



表1.2 是此函数第二个参数flag可能的常量值及其含义。如需传递多个参数，则应通过位或运算（OR）符组合并传递

| 打开模式 | 含义                       |
| -------- | -------------------------- |
| O_CREAT  | 必要时创建文件             |
| O_TRUNC  | 删除全部现有数据           |
| O_APPEND | 维持现有数据，保存到其后面 |
| O_RDONLY | 只读打开                   |
| O_WRONLY | 只写打开                   |
| O_RDWR   | 读写打开                   |

#### 1.2.3 关闭文件

使用文件后必须关闭，释放对应的文件描述符。

```c
#include <unistd.h>

int close(int fd);
/*
	成功时返回0，失败时返回-1.
	
	fd为需要关闭的文件或套接字的文件描述符
*/
```

此函数不仅可以关闭文件，还可以关闭套接字，再次证明了"Linux操作系统 不区分文件与套接字"的特点。



#### 1.2.4 将数据写入文件

接下来介绍的write函数用于向文件传输数据，当然，Linux中不区分文件与套接字，因此，通过套接字向其他计算机传递数据时也会用到该函数。

```c
#include <unistd.h>
ssize_t write(int fd, const void* buf, size_t nbytes);
/*
	成功时返回写入的字节数，失败时返回-1
	
	fd 显示数据传输对象的文件描述符
	buf 保存要传输数据的缓冲区地址值
	nbytes 要传输数据的字节数
*/
```

次函数定义中，size_t是通过typedef声明的unsigned int类型。对ssize_t来说，size_t前面多加的s代表signed，即ssize_t是通过typedef声明的signed int类型。

> **以_t为后缀的数据类型**
>
> 我们已经接触到ssize_t，size_t等陌生的数据类型，这些都是元数据类型，在sys/types.h头文件中一般由typedef声明定义。
>
> 作用：早期int为16位的，但随着时代的变化，现在大多数操作系统把int定义为32位。如果之前已在需要声明4字节数据之处使用了size_t或sszie_t，则将大大减少代码的变动，因为只需要修改并编译size_t和ssize_t的typedef声明即可。
>
> 在项目中，为了给基本数据类型赋予别名，一般会添加大量typedef声明。而为了与程序员定义的新数据类型加以区分，操作系统定义的数据类型会添加后缀_t。



下面通过示例帮助大家更好地理解前面讨论过的函数。此程序将创建新文件并保存数据。

写数据程序代码参见：[low_open.c](https://github.com/wangjunstf/TCP-IP-Network-Note/blob/main/ch01-%E7%90%86%E8%A7%A3%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B%E5%92%8C%E5%A5%97%E6%8E%A5%E5%AD%97/src/low_open.c)

```c
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>

void errorHandling(const char* message);

int main(){
    int fd;
    char buf[] = "Let's go!\n";
    
    //文件打开模式为O_CREAT，O_WRONLY，O_TRUNC的组合，因此将创建空文件，并只能写。
    //如果存在data.txt文件，则清空文件的全部数据
    fd = open("data.txt", O_CREAT|O_WRONLY|O_TRUNC);
    if(df == -1){
        errorHandling("open() error!");
    }
    printf("file descriptor: %d \n", fd);
    
    //向对应于fd中保存的文件描述符的文件传输buf中保存的数据
    if(write(fd, buf, sizeof(buf))==-1)
        errorHandling("write() error!");

    close(fd);
    return 0;
}

void errorHandling(const char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
```

**编译并运行**

```shell
gcc low_open.c -o lopen
./lopen
#输出“file descriptor: 3”
#利用Linux的cat命令输出data.txt文件的内容，可以确认确实已向文件传输数据
cat data.txt        
#输出“Let's go!”
```



#### 1.2.5读取文件中的数据

与之前的write函数相对应，read函数用来输入（接收）数据。

```c
#include <unistd.h>

ssize_t read(inf fd, void* buf, size_t nbytes);
/*
	成功时返回接收的字节数（但遇到文件结尾则返回0），失败时返回-1
	
	fd 显示数据接收对象的文件描述符
	buf 要保存接收数据的缓冲地址值
	nbytes 要接收数据的最大字节数
*/
```

下列示例将通过read函数读取data.txt中保存的数据。

代码参见：[low_read.c](https://github.com/wangjunstf/TCP-IP-Network-Note/blob/main/ch01-%E7%90%86%E8%A7%A3%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B%E5%92%8C%E5%A5%97%E6%8E%A5%E5%AD%97/src/low_read.c)



**编译并运行**

```shell
gcc low_read.c -o lread
./lread
#输出以下内容：
#file descriptor:3 
#file data: Let's go!

```

基于文件描述符的I/O操作相关介绍到此结束。希望各位记住，该内容同样适用于套接字。

#### 1.2.6 文件描述符与套接字

下面将同时创建文件和套接字，并用整数形态比较返回的文件描述值。

代码参见： [fd_seri.c](https://github.com/wangjunstf/TCP-IP-Network-Note/blob/main/ch01-%E7%90%86%E8%A7%A3%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B%E5%92%8C%E5%A5%97%E6%8E%A5%E5%AD%97/src/fd_seric.c)

```shell
gcc fd_seric.c  -o fds
./fds
#输出以下内容
#file descriptor 1: 3
#file descriptor 2: 4
#file descriptor 3: 5
```

从输出的文件描述整数值可以看出，描述符从3开始由小到大顺序编号，因为0,1,2时分配给标准I/O的描述符。



### 1.3 基于Windows平台的实现

暂略

### 1.5 习题

（1）套接字在网络编程中的作用是什么？为何称它为套接字？

> 套接字是操作系统提供的**部件**，是网络数据传输用的**软件设备**，用于**网络数据传输**。
>
> 我们把插头插到插座上能从电网获得电力供给，同样，我们为了与远程计算机进行数据传输，需要连接因特网，而编程中的“套接字”就是用来连接该网络的工具。



（2）在服务器端创建套接字以后，会依次调用 listen 函数和 accept 函数。请比较二者作用。

> 调用listen将套接字转为可接收连接状态（监听状态），此时该进程会进入阻塞态，即不再向下执行，有连接请求后，程序继续往下执行，调用accept函数处理此次连接。



（3）Linux 中，对套接字数据进行 I/O 时可以直接使用文件 I/O 相关函数；而在 Windows 中则不可以。原因为何

> Linux中，一切皆文件。在Linux世界里，socket也被认为是文件的一种，socket操作与文件操作没有区别，因此在网络数据传输过程中，可以直接使用I/O相关函数。
>
> windows里，socket和文件不是一个概念，当然就不能直接使用文件I/O函数来操作socket。



（4）创建套接字后一般会给他分配地址，为什么？为了完成地址分配需要调用哪个函数？

> 当我们与其它主机进行数据传输时，我们得知道目标主机的地址，目标主机回应消息也需要我们的地址。对套接字来说，地址就是ip地址和端口号。
>
> 调用bind函数分配地址。



（5）Linux 中的文件描述符与 Windows 的句柄实际上非常类似。请以套接字为对象说明它们的含义。

> 略

（6）底层 I/O 函数与 ANSI 标准定义的文件 I/O 函数有何区别？

> 文件 I/O 又称为低级磁盘 I/O，遵循 POSIX 相关标准。任何兼容 POSIX 标准的操作系统上都支持文件I/O。标准 I/O 被称为高级磁盘 I/O，遵循 ANSI C 相关标准。只要开发环境中有标准 I/O 库，标准 I/O 就可以使用。（Linux 中使用的是 GLIBC，它是标准C库的超集。不仅包含 ANSI C 中定义的函数，还包括 POSIX 标准中定义的函数。因此，Linux 下既可以使用标准 I/O，也可以使用文件 I/O）。

（7）参考本书给出的示例`low_open.c`和`low_read.c`，分别利用底层文件 I/O 和 ANSI 标准 I/O 编写文件复制程序。可任意指定复制程序的使用方法。

**底层文件 I/O**

示例代码： 

[low_open.c](https://github.com/wangjunstf/TCP-IP-Network-Note/blob/main/ch01-%E7%90%86%E8%A7%A3%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B%E5%92%8C%E5%A5%97%E6%8E%A5%E5%AD%97/src/low_open.c)

[low_read.c](https://github.com/wangjunstf/TCP-IP-Network-Note/blob/main/ch01-%E7%90%86%E8%A7%A3%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B%E5%92%8C%E5%A5%97%E6%8E%A5%E5%AD%97/src/low_read.c)

**ANSI 标准 I/O**

示例代码：

[ansi_open.c](https://github.com/wangjunstf/TCP-IP-Network-Note/blob/main/ch01-%E7%90%86%E8%A7%A3%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B%E5%92%8C%E5%A5%97%E6%8E%A5%E5%AD%97/src/ansi_open.c)

[ansi_read.c](https://github.com/wangjunstf/TCP-IP-Network-Note/blob/main/ch01-%E7%90%86%E8%A7%A3%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B%E5%92%8C%E5%A5%97%E6%8E%A5%E5%AD%97/src/ansi_read.c)







