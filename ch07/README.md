

# 第7章 优雅地断开套接字连接

TCP中的断开连接过程往往比连接过程更重要，连接过程往往不会出什么变数，但断开过程有可能发生预想不到的情况。只有掌握了半关闭，才能明确断开过程。

## 7.1 基于TCP的半关闭

### 单方面断开连接带来的问题

Linux的close函数意味着完全断开连接，完全断开不仅指无法传输数据，也无法接收数据。在某些情况下，通信一方调用close函数断开连接就显得不太优雅。

![单方面断开连接](https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E5%8D%95%E6%96%B9%E9%9D%A2%E6%96%AD%E5%BC%80%E8%BF%9E%E6%8E%A5.png)





### 套接字和流

两台主机通过套接字建立连接后进入可交换数据的状态，又称为"流形成的状态"。也就是把建立套接字后可交换数据的状态看作一种流。

![套接字中生成的两个流](https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E5%A5%97%E6%8E%A5%E5%AD%97%E4%B8%AD%E7%94%9F%E6%88%90%E7%9A%84%E4%B8%A4%E4%B8%AA%E6%B5%81.png)



本章讨论的"优雅地断开连接方式"只断开其中1个流，而非同时断开两个流。



### 针对优雅断开的shutdown函数

shutdown函数用来关闭套接字的一个流。

```c
#include <sys/socket.h>

int shutdown(int sock, int howto);
//成功时返回0，失败时返回-1

/*
	需要断开的套接字文件描述符
	howto 传递断开方式信息
*/
```



howto的可选值：

* SHUT_RD:  断开输入流
* SHUT_WR:  断开输出流
* SHUT_RDWR:  同时断开I/O流

### 为何需要半关闭

当服务器端向客户端发送一个文件时，发送完毕后，需要得到客户端的反馈以确认文件是否正确送达。对服务器端而言，没什么问题，它只要把文件发出即可。

但对于客户端而言，需要考虑到：每次从服务器端接收文件的一部分，接收多少次才能接收整个文件。针对这个问题，可以向客户端传递一个特殊符号EOF来表示文件的结束，但服务器端如何传输EOF，一个有效办法就是通过客户端接收函数的返回值。

> 断开输出流时向对方主机传输EOF。

服务器端只需断开输出流，这样就告诉客户端文件传输完毕，同时服务器端也能收到客户端接收成功的返回信息。



## 7.2 基于半关闭的文件传输程序

客户端向服务器端发出连接请求，连接成功后，服务器端将文件file_server.c发送给客户端，客户端接收成功后，给服务器端发送消息: "Thank you"。

### 源代码

file_server.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30
void error_handling(char* message);

int main(int argc, char* argv[]){
    int serv_sd,clnt_sd;
    FILE *fp;
    char buf[BUF_SIZE];
    int read_cnt;

    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz; //记录客户端地址结构体长度
    if(argc != 2){
        printf("Usage: %s <PORT>\n", argv[0]);
        exit(1);    
        //立即终止调用过程，属于该进程的所有文件描述符都被关闭，
        //并且该进程的任何子进程都由进程1(init)继承，并且向该进程的父进程发送SIGCHLD信号。
    }

    fp = fopen("file_server.c","rb");   //以二进制只读方式打开，该文件必须存在
    serv_sd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    bind(serv_sd, (struct sockaddr *)&serv_adr, sizeof(serv_adr));
    listen(serv_sd,5);   //开启监听状态，等待队列数为5

    clnt_adr_sz = sizeof(clnt_adr);
    clnt_sd = accept(serv_sd, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

    while(1){
        read_cnt = fread((void *)buf,1, BUF_SIZE, fp);
        if(read_cnt<BUF_SIZE){
            write(clnt_sd, buf, read_cnt); //每次读取BUF_SIZE，当读取的字节数不足BUF_SIZE，说明已经读取完毕了
            break;
        }
        write(clnt_sd, buf,BUF_SIZE);
    }

    shutdown(clnt_sd,SHUT_WR);
    read_cnt = read(clnt_sd, buf, BUF_SIZE);
    buf[read_cnt] = 0;
    printf("%s\n",buf);
    fclose(fp);
    shutdown(clnt_sd, SHUT_RD);
    close(serv_sd);
    

    return 0;
}

void error_handling(char* message){
    fputs("message",stderr);
    fputc('\n',stderr);
    exit(1);
}
```



file_client.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30

void error_handling(char* message);

int main(int argc, char* argv[]){
    int sd;
    FILE *fp;

    char buf[BUF_SIZE];
    int read_cnt;
    struct sockaddr_in serv_adr;    //客户端地址结构体

    if(argc!=3){
        printf("Usage: %s <IP> <PORT>\n",argv[0]);
        exit(1);
    }

    fp = fopen("receive.dat","wb");  //以只写二进制方式创建一个文件并打开，若已存在同名文件，则丢弃该文件所有数据，并打开
    sd = socket(PF_INET, SOCK_STREAM,0);

    memset(&serv_adr, 0 , sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if(connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1){
        error_handling("connect() error");
    }

    while((read_cnt = read(sd, buf, BUF_SIZE))!= 0){
        fwrite((void*)buf, 1, read_cnt, fp);
    }

    puts("receive file data");
    write(sd,"Thank you!",11);
    fclose(fp);
    close(sd);
    return 0;
}

void error_handling(char* message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}
```



### 编译运行

服务器端

```shell
$ gcc file_server.c -o file_server
$ ./file_server 9190
Thank you!
```



客户端

```shell
$ gcc file_client.c  -o file_client
$ ./file_client 127.0.0.1 9190
receive file data
```



执行完毕后，服务器端收到消息"Thank you!",  客户端所在文件夹多了新文件"receive.dat",并打印出"receive file data".



### 7.3 习题

（1）解释TCP中“流”的概念。UDP中能否形成流？请说明原因。

> 答：TCP中的流，就是服务器端与客户端建立连接后，形成两条流，分别为输入流和输出流。客户端的输出流对应服务器端的输入流，客户端的输出流对应服务器端的输出流。
>
> 不能，因为UDP不需要建立连接，通过完整数据包直接传输数据。

（2） Linux中的close函数或Windows中的closesocket函数属于单方面断开连接的方法，有可能带来一些问题。什么是单方面断开连接？什么情况下会出现问题？

> 答：会让服务器端/客户端都无法再发送数据和接收数据。当服务器/客户端需要向对方发送EOF，并且需要得到对方的回应时，调用close后就会有问题，这时就接收不到回复消息了。



（3）什么是半关闭？针对输出流执行半关闭的主机处于何种状态？半关闭会导致对方主机接收什么信息？

> 答：半关闭是指只关闭输入流或输出流。
>
> 只能接收数据，不能发送数据。
>
> 接收到EOF