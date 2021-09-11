# 第 24 章 制作HTTP服务器端

## 1、HTTP概要

### 1.1 连接web服务器

“基于HTTP协议，将网页对应文件传输给客户端的服务器”。

HTTP协议：以超文本传输为目的而设计的应用层协议，这种协议基于TCP/IP实现的协议，我们也可以自己实现HTTP。



### 1.2 HTTP

#### 无状态的Stateless协议

为了在网络环境中同时向大量客户端提供服务，HTTP协议的请求及响应方式设计如图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-23%20%E4%B8%8B%E5%8D%885.25.14.png" alt="截屏2021-05-23 下午5.25.14" style="zoom:50%;" />



从上图可以看出，服务器端响应客户端请求后立即断开了连接。即使同一客户端再次发送请求，服务器端也无法分辨出是原先哪一个，而会以相同的方式处理新请求。因此HTTP又称“无状态的Stateless协议”



为了弥补HTTP无法保持连接的缺点，web编程中通常会使用Cookie和Session技术。

> Cookie：类型为“小型文本文件”，指某些网站为了辨别用户身份而储存在用户本地终端（Client Side）上的数据（通常经过加密）。cookie采用的是键值对结构存储，只不过键和值都是字符串类型。
>
> Session：是服务器程序记录在服务器端的会话消息。



#### 请求消息（Request Message）的结构

Web服务器端需要解析并响应客户端请求。客户端和服务端之间的数据请求方式标准如下图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-23%20%E4%B8%8B%E5%8D%885.38.25.png" alt="截屏2021-05-23 下午5.38.25" style="zoom:50%;" />



请求消息可分为请求行，消息头，消息体等3个部分。

请求行中含有请求方式消息，典型的请求方式有GET和POST。GET主要用于请求数据，POST主要用于传输数据

"GET/index.html HTTP/1.1" 具有如下含义：

请求(GET)index.html文件，希望以1.1版本的HTTP协议进行通信。

请求行只能通过1行(line)。因此服务器端很容易从HTTP请求中提取第一行，并分析请求行中的信息。



#### 响应消息（Response Message）消息

下面介绍Web服务器端向客户端传递的响应消息的结构。响应消息由状态行，头消息，消息体等3个部分构成。该响应消息由状态行，头消息，消息体等3部分构成。状态行中含有关于请求的状态消息。

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-23%20%E4%B8%8B%E5%8D%886.05.42.png" alt="截屏2021-05-23 下午6.05.42" style="zoom: 50%;" />



例如客户端请求index.html文件时，表示index.html文件是否存在，服务器端是否发生问题而无法响应等不同情况的信息将写入状态行。

"HTTP/1.1 200 OK"的含义为：我想用HTTP1.1版本进行响应，你的请求已正确处理(200 OK)

表示“客户端请求的执行结果”的数字称为状态码，典型的有以下几种。

200 OK：成功处理了请求

404 Not Found：请求的文件不存在

400 Bad Request：请求方式错误，请检查

消息头中含有传输的数据类型和长度等信息

最后插入一个空行，通过消息体发送客户端请求的文件数据。



## 2、实现简单的Web服务器端

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define SMALL_BUF 100

void* request_handler(void* arg);
void send_data(FILE* fp, char* ct, char* file_name);
char* content_type(char* file);
void send_error(FILE* fp);
void error_handling(char* message);

int main(int argc, char* argv[]){
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_size;
    char buf[BUF_SIZE];
    pthread_t t_id;
    if(argc != 2){
        printf("Usage: %s <PORT>\n",argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));
    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
        error_handling("bind error");
    
    if(listen(serv_sock, 20)==-1)
        error_handling("listen() error");
    
    while(1){
        clnt_adr_size = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_size);
        printf("Connect Request : %s:%d\n", inet_ntoa(clnt_adr.sin_addr), ntohs(clnt_adr.sin_port));
        pthread_create(&t_id, NULL, request_handler, &clnt_sock);
        pthread_detach(t_id);
    }
    close(serv_sock);
    return 0;
}

void * request_handler(void* arg){
    int clnt_sock = *((int*)arg);
    char req_line[SMALL_BUF];
    FILE* clnt_read;
    FILE* clnt_write;

    char method[10];
    char ct[15];
    char file_name[30];
    clnt_read = fdopen(clnt_sock, "r");        // fdopen 将文件描述符转为FILE指针
    clnt_write = fdopen(dup(clnt_sock), "w");  // dup 为复制文件描述符，
    fgets(req_line, SMALL_BUF, clnt_read);     // fgets从指定流中读取一行
    if(strstr(req_line, "HTTP/")==NULL){
        send_error(clnt_write);
        fclose(clnt_read);
        fclose(clnt_write);
        return NULL;
    }
    strcpy(method, strtok(req_line, " /"));
    strcpy(file_name, strtok(NULL, " /"));
    strcpy(ct, content_type(file_name));
    if(strcmp(method, "GET")!=0){
        send_error(clnt_write);
        fclose(clnt_write);
        fclose(clnt_read);
        return NULL;
    }
    fclose(clnt_read);
    send_data(clnt_write, ct, file_name);
}

void send_data(FILE* fp, char* ct, char* file_name){
    char path[] = "./";
    char protocol[] = "HTTP/1.0 200 OK\r\n";
    char server[] = "Server: Linux Web Server \r\n";
    char cnt_len[] = "Content-length:2048\r\n";
    char cnt_type[SMALL_BUF];
    FILE * send_file;
    char buf[BUF_SIZE];
    sprintf(cnt_type, "Content-type:%s\r\n\r\n",ct);
    char *file_path = malloc(sizeof(char) * (strlen(file_name) + strlen(path) + 1));
    strcpy(file_path, path);
    strcat(file_path, file_name);

    send_file = fopen(file_path,"r");
    if(send_file==NULL){
        send_error(fp);
        return;
    }
    // 传输头信息
    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_len,fp);
    fputs(cnt_type, fp);

    // 传输请求数据
    while(fgets(buf, BUF_SIZE, send_file)!=NULL){
        fputs(buf, fp);
        fflush(fp);
    }
    fflush(fp);
    fclose(fp);
}

char * content_type(char* file){
    char extension[SMALL_BUF];
    char file_name[SMALL_BUF];
    strcpy(file_name, file);
    strtok(file_name, ".");
    strcpy(extension, strtok(NULL, "."));

    if(!strcmp(extension, "html") || strcmp(extension, "htm"))
        return "text/html";
    else
        return "text/plain";
}

void send_error(FILE *fp){
    char protocol[] = "HTTP/1.0 404 Not Found\r\n";
    char date[] = " Tue, 10 Jul 2012 06:50:15 GMT\r\n";
    char cnt_len[] = "Content-Length: 2048\r\n";
    char cnt_type[] = "Content-Type: text/html;charset=utf-8\r\n\r\n";

    char server[] = "Server: Linux Web Werver \r\n";
    char content[] = "<html><head><meta charset=\"utf-8\"><title>NETWORK</title></head>"
                                                          "<body><front size=+5><br>发生错误！查看请求文件名和请求方式！</front></body>"
                                                          "</html>";
    fputs(protocol, fp);
    fputs(date, fp);
    fputs(server, fp);
    fputs(cnt_len,fp);
    fputs(cnt_type, fp);
    fputs(content, fp);
    fflush(fp);
}

void error_handling(char* message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);

}
```



编译运行

```shell
$ gcc -g  webserver_linux.c -D_REENTRANT -o ./bin/webserver_linux -lpthread
$ ./bin/webserver_linux  9190
Connect Request : 10.211.55.2:55445
Connect Request : 10.211.55.2:55450
```

接下来就可以通过浏览器访问了

http://10.211.55.22:9190/index.html



## 3、习题（参考答案）

![截屏2021-05-23 下午6.27.23](https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-23%20%E4%B8%8B%E5%8D%886.27.23.png)

![截屏2021-05-23 下午6.23.06](https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-23%20%E4%B8%8B%E5%8D%886.23.06.png)



（1）a，b：服务器端响应客户端请求后立即断开连接，e

（2）a，目前广泛使用HTTP协议基于TCP协议，只能使用TCP协议实现。下一代HTTP3，基于UDP，可以使用UDP协议实现。

（3）因为HTTP协议是"无状态的states协议"，每次请求都需要经过套接字的创建和销毁过程。

epoll对多核/多线程的支持不够好,