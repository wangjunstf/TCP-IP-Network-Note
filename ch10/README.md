# 第10章 多进程服务器端

## 1、进程的概念及应用

### 1.1 两种类型的服务器端

单进程下，每次只能受理一个客户端的请求，第1位连接请求的受理时间是0秒，第50位连接请求的受理时间是50秒，第100位连接请求的受理时间是100秒。很显然这种方式是不合理的，没有多少人有耐心为了一个页面请求等待几分钟。



多进程下，为每个请求新建一个进程，在宏观上相当于同时受理所有客户端的连接请求，每个客户端连接请求的受理时间为2～3秒，这种技术更加合理一些。



### 1.2 并发服务器端的实现方法

网络程序中数据通信时间比CPU运行时间占比更大，因此，向多个客户端提供服务是一种有效利用CPU的方式。下面是具有代表性的并发服务器端实现模型和方法：

* 多进程服务器：通过创建多个进程提供服务。
* 多路复用服务器：通过捆绑并统一管理I/O对象提供服务。
* 多线程服务器：通过生成与客户端等量的线程提供服务。

本章重点介绍多进程服务器。

### 1.3 理解进程

#### 进程的概念

进程（Process）是计算机中的程序关于某数据集合上的一次运行活动，是系统进行资源分配和调度的基本单位，是操作系统结构的基础。

简单说："进程就是占用空间的正在运行的程序!"

一个可执行程序，在开始运行前只是一个普通程序，即存放在磁盘中，包含二进制代码的文件。当运行该程序后，系统将该程序调入内存，就变为一个进程(包含二进制代码，及所使用的各种计算机资源)，该进程可以申请或释放计算机资源。

#### 单核和多核

单核cpu一次只能运行一个进程，当现代操作系统都采用分时使用cpu资源，即每个进程轮流使用cpu，只不过每个进程执行时间很短，所以造成了进程同时执行的错觉。

#### 进程查询

多核cpu可以同时执行多个进程，但进程数量往往大于核心数，所以进程往往都需要分时使用CPU资源。

```shell
$ ps au
USER       PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
gdm       1450  0.0  0.2 212136  6056 tty1     Ssl+ Apr29   0:00 /usr/lib/gdm3/gdm-x-sessio
root      1452  0.0  0.7 275792 16044 tty1     Sl+  Apr29   0:28 /usr/lib/xorg/Xorg vt1 -di
gdm       1514  0.0  0.6 558988 13992 tty1     Sl+  Apr29   0:00 /usr/lib/gnome-session/gno
gdm       1532  0.0  6.5 3101252 133240 tty1   Sl+  Apr29   1:45 /usr/bin/gnome-shell
gdm       1555  0.0  0.3 435100  7836 tty1     Sl   Apr29   0:00 ibus-daemon --xim --panel 
gdm       1558  0.0  0.2 280748  5912 tty1     Sl   Apr29   0:00 /usr/lib/ibus/ibus-dconf
gdm       1562  0.0  1.0 344012 20832 tty1     Sl   Apr29   0:00 /usr/lib/ibus/ibus-x11 --k
gdm       1589  0.0  1.0 494596 21748 tty1     Sl+  Apr29   0:00 /usr/lib/gnome-settings-da
gdm       1594  0.0  0.2 278160  5788 tty1     Sl+  Apr29   0:00 /usr/lib/gnome-settings-da
gdm       1596  0.0  0.9 343612 20112 tty1     Sl+  Apr29   0:00 /usr/lib/gnome-settings-da
```

使用ps命令可以查看当前系统正在运行的进程，每个进程都具有唯一的编号(PID)，一个系统能使用的PID数量是有限的，因而一个系统能使用的进程是有限的。



### 1.4 进程创建方法

创建进程的方法有很多，此处介绍用fork函数创建子进程。

```shell
#include <unistd.h>

pid_t fork(void);
//成功时返回进程IP，失败时返回-1
```

fork函数将创建调用的进程副本。也就是说：并非完全根据不同的程序创建进程，而是复制正在运行的，调用fork函数的进程。另外，两个函数都将执行fork函数调用后的语句（准确地说是在fork函数返回之后）。但因为通过同一个进程，复制相同的内存空间，之后的程序流要根据fork函数的返回值加以区分。即利用fork函数的如下特点区分程序的执行流程。

* 父进程：fork函数返回子进程ID
* 子进程：fork函数返回0

此处“父进程”指原进程，即调用fork函数的主体，二“子进程是通过父进程调用fork函数”复制出的进程。下图表示fork函数后的程序运行流程。

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/fork%E5%87%BD%E6%95%B0%E7%9A%84%E8%B0%83%E7%94%A8.png" alt="fork函数的调用" style="zoom: 33%;" />



子进程和父进程都拥有执行fork函数之前的变量，但它们的运行不会相互影响。

下面给出例子验证之前的猜想：

#### 代码

```c
#include <stdio.h>
#include <unistd.h>

int gval = 10;
int main(int argc, char* argv[]){
    __pid_t pid;      //原书为pid_t，pid_t与__pid_t是的等价的
    int lval = 20;
    gval++, lval+=5;
    
    pid = fork();
    if(pid==0){
        gval==2, lval+=2;
    }else{
        gval-=2,lval-=2;
    }

    if(pid==0)
        printf("Child Proc: [%d, %d]\n",gval, lval);
    else
        printf("Parent Proc: [%d, %d]\n",gval, lval);

    return 0;
}
```



#### 编译运行

```shell
$ gcc fork.c -o ./bin/fork
$ ./bin/fork
Parent Proc: [9, 23]
Child Proc: [11, 27]
```



## 2、僵尸进程

### 2.1 概念

文件操作中关闭文件和打开文件同等重要。同样，进程销毁也和进程创建同等重要。如果未认真对待进程销毁，它们就会变成僵尸进程。

僵尸进程是当子进程比父进程先结束，而父进程又没有回收子进程，释放子进程占用的资源，此时子进程将成为一个僵尸进程。如果父进程先退出 ，子进程被init接管，子进程退出后init会回收其占用的相关资源。

### 2.2 产生原因

调用fork函数产生子进程的终止方式：

* 传递参数并调用exit()函数
* main函数中执行return语句并返回值

向exit函数传递的参数值和main函数的return语句返回的值都会传递给操作系统，而操作系统不会销毁子进程，直到把这些值传递给产生该子进程的父进程。处在这种状态下的进程就是**僵尸进程**。也就是说，是操作系统将子进程变成僵尸进程。那么，此进程何时被销毁呢？

**应该向创建子进程的父进程传递子进程的exit参数或return语句的返回值**

当然还没完，操作系统不会把这些值主动传递给父进程，只有父进程主动发起请求(函数调用)时，操作系统才会传递该值。

下面例子演示僵尸进程的产生：

#### 源码

```c
#include <stdio.h>
#include <unistd.h>

int gval = 10;
int main(int argc, char* argv[]){
    __pid_t pid;
    int lval = 20;
    gval++, lval+=5;
    
    pid = fork();
    if(pid==0){
        gval==2, lval+=2;
    }else{
        gval-=2,lval-=2;
    }

    if(pid==0)
        printf("Child Proc: [%d, %d]\n",gval, lval);
    else
        printf("Parent Proc: [%d, %d]\n",gval, lval);

    return 0;
}
```



#### 编译运行

```shell
$ gcc zombie.c -o ./bin/zombie
$ ./bin/zombie
Child Process ID : 5399 
Hi, I am a child process
End child process
END parent process
```

```shell
# 在另一个终端窗口中执行
$ ps au
mygit     5482  0.0  0.2  29808  5032 pts/3    Ss   12:39   0:00 /bin/bash
mygit     5532  0.0  0.0   4512   824 pts/2    S+   12:40   0:00 ./bin/zombie
mygit     5533  0.0  0.0      0     0 pts/2    Z+   12:40   0:00 [zombie] <defunct>
```

当父进程处于睡眠状态时，子进程为僵尸态。



上面的结果需要用另一个终端窗口来验证进程的状态，也可以将zombie后台运行，这样就只需要一个窗口就可以完成验证。`$ ./bin/zombie & `即可在后台运行



## 3 销毁僵尸进程

### 3.1 利用wait函数

```c
#include <sys/wait.h>

pid_t wait(int *statloc);
//成功时返回终止的子进程ID，失败时返回-1
```



调用次函数时如果已有子进程终止，那么子进程终止时传递的返回值(exit函数的参数值，main函数的return返回值)将保存到该函数的参数所指内存空间。但函数参数指向的单元中还包含其他信息，因此需要通过下列宏进行分离。

* WIFEXITED子进程正常终止时返回true
* WEXITSTATUS返回子进程的返回值

也就是，向wait函数传递变量status的地址时，调用wait函数后应该编写如下代码。

```
if(WIFEXITED(status)){
	puts("Normal termination!");
	printf("Child pass num: %d",WEXITSTATUS(status));
}
```

**调用wait函数时若没有子进程终止，那么程序将阻塞，直到有子进程终止，因此需谨慎调用该函数**

#### 演示代码

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc,char* argv[]){
    int status;
    pid_t pid = fork();

    if(pid==0){
        return 3;
    }else{
        printf("Child PID: %d \n",pid);
        pid = fork();
        if(pid==0){
            exit(7);
        }else{
            printf("Child PID: %d \n",pid);
            wait(&status);
            if(WIFEXITED(status))
                printf("Child send one: %d \n",WEXITSTATUS(status));

            wait(&status);
            if (WIFEXITED(status))
                printf("Child send one: %d \n", WEXITSTATUS(status));

            sleep(30);
        }
    }
}
```



#### 编译运行

```shell
$ gcc wait.c  -o ./bin/wait
$ ./bin/wait 
Child PID: 19403 
Child PID: 19404 
Child send one: 3 
Child send one: 7 
```

```shell
# 在另一个终端窗口中执行
$ ps au
mygit    10870  0.0  0.2  29940  5260 pts/0    Ss   23:11   0:00 /bin/bash
root     13828  0.0  0.1  72248  3836 pts/1    S    14:03   0:00 su - root
root     13835  0.0  0.2  29952  5372 pts/1    S+   14:03   0:00 -su
mygit    19756  0.0  0.0   4512   752 pts/0    S+   23:37   0:00 ./bin/wait
mygit    19787  0.0  0.1  46776  3620 pts/4    R+   23:37   0:00 ps au

# 可以发现并未产生僵尸进程
```



### 3.2 利用waitpid函数

wait函数会引起程序阻塞，还可以考虑使用waitpid函数。

```c
#include <sys/wait.h>

pid_t waitpid(pid_t pid, int *statloc, int options);
//成功时返回终止的子进程ID(或0)，失败时返回-1

/*
	pid 等待终止的目标子进程的ID，若传递-1， 则与wait函数相同，可以等待任意子进程终止
	statloc 与wait函数的statloc具有相同的含义
	options 传递头文件sys/wait.h中声明的常量WNOHANG，即使没有终止的子进程也不会进入阻塞状态，而是返回0并退出函数
*/
```

#### 演示代码

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]){
    int status;
    pid_t pid = fork();
    if(pid==0){
        sleep(15);
        return 24;
    }else{
        while(!waitpid(-1, &status,WNOHANG)){
            sleep(3);
            puts("sleep 3sec.");
        }

        if(WIFEXITED(status)){
            printf("Child send %d \n",WEXITSTATUS(status));
        }
    }
    return 0;
}c
```

#### 编译运行

```shell
$ gcc waitpid.c -o ./bin/waitpid
$ ./bin/waitpid 
sleep 3sec.
sleep 3sec.
sleep 3sec.
sleep 3sec.
sleep 3sec.
Child send 24 
```



## 4、信号处理

“子进程究竟何时终止？调用waitpid函数后要无休止地等待吗？”

父进程往往与子进程一样繁忙，因此不能只调用waitpid函数以等待子进程终止。

### 4.1 向操作系统求助

子进程终止的识别主体是操作系统，因此，若操作系统在子进程结束时，能给它的父进程发一个消息，"嘿，父进程，你创建的子进程终止了！"，此时父进程将暂时放下工作，处理子进程终止的相关事宜。这应该更加合理。为了实现该想法，我们引入了信号处理(Signal Handling)机制。

此处“信号”是指在特定事件发生时由操作系统向进程发送的消息。

另外，为了响应消息，执行与消息相关的自定义操作的过程称为“信号处理”。



### 4.2 信号与signal函数

为了销毁终止的进程，需要父进程提前设定好一个"信号处理函数"，当进程终止时，由操作系统执行该函数。

```c
#include <signal.h>

void (*signal(int signo, void (*func)(int)))(int);
// 为了在产生信号时调用，返回之前注册的函数指针
/*
	函数名：signal
	参数：int signo, void (*func)(int)
	返回类型：参数为int型，返回void型指针
*/
```

signo的可选值：

SIGALRM：已到通过调用alarm函数注册的时间。

SIGINT：输入CTRL+C

SIGCHLD：子进程终止



接下来编写signal函数的调用语句，分别完成以下两个请求：

1. 已到通过alarm函数注册的时间，请调用timeout函数
2. 输入CTRL+C时调用keycontrol函数

很简单，编写完相应的函数，调用signal函数即可

signal(SIGALRM, timeout);

signal(SIGINT, keycontrol);



alerm函数声明

```c
#include <unistd.h>

unsigned int alarm(unsigned int seconds);
// 返回0或以秒为单位的距SIGALRM信号发生所剩时间
// 若传递0，则之前对SIGALRM信号的预约取消#include <unistd.h>
```

#### 演示代码

signal.c

```c
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

void timeout(int sig){
    if(sig == SIGALRM){
        puts("Time out!");
    }

    alarm(2);
}

void keycontrol(int sig){
    if(sig == SIGINT){
        puts("CTRL+C pressed");
    }
}

int main(int argc, char* argv){
    signal(SIGALRM,timeout);
    signal(SIGINT, keycontrol);

    alarm(2);
    for(int i=0; i<3; i++){
        puts("wait...");
        sleep(100);
    }
    return 0;
}
```

#### 编译运行

```shell
$ gcc signal.c -o ./bin/signal
$ ./bin/signal 
wait...
Time out!
wait...
Time out!
wait...
Time out!
```

puts("wait...")执行之后，并没有休眠100秒，而是在2秒后就输出了Time out!，因为**发生信号时将唤醒由于调用sleep函数而进入阻塞状态的进程**



### 4.3 利用sigaction函数进行信号处理

signal函数与sigaction函数的区别：signal函数在UNIX系列的不同操作系统中可能存在区别，但sigaction函数完全相同。

实际开发中很少用signal函数编写程序，只是为了与旧版程序的兼容。为了更好的稳定性，应该使用sigaction函数进行信号处理。

sigaction函数声明

```c
#include <signal.h>

int sigaction(int signo, const struct sigaction *act, struct sigaction *oldact);
//成功时返回0，失败时返回-1

/*
	signo 与signal函数相同，传递信号信息
	act 信号处理函数
	oldact 获取之前注册的信号处理函数指针，若不需要则传递0
*/
```

声明并初始化sigaction结构体变量以调用上述函数

```c
struct sigaction{
	void (*sa_handler)(int);     //信号处理函数
  sigset_t sa_mask;						 
  int sa_flags;
}
```

sigaction.c

#### 演示代码

```c
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

void timeout(int sig)
{
    if (sig == SIGALRM)
    {
        puts("Time out!");
    }

    alarm(2);
}

void keycontrol(int sig)
{
    if (sig == SIGINT)
    {
        puts("CTRL+C pressed");
    }
}

int main(int argc, char *argv)
{
    struct sigaction act;
    act.sa_handler = timeout;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGALRM, &act, 0);
    alarm(2);
    for (int i = 0; i < 3; i++)
    {
        puts("wait...");
        sleep(100);
    }
    return 0;
}
```

#### 编译运行

```shell
$ gcc sigaction.c -o ./bin/sigaction
$ ./bin/sigaction 
wait...
Time out!
wait...
Time out!
wait...
Time out!
```



## 5、利用信号处理技术消灭僵尸进程

子进程终止时将产生SIGCHLD信号，接下来利用sigaction函数编写实例：

#### 代码示例 

remove_zombie.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

void read_childproc(int sig){
    int status;
    pid_t id = waitpid(-1, &status, WNOHANG); //WNOHANG为
    if(WIFEXITED(status)){
        printf("Remove oroc id: %d\n", id);
        printf("Child send: %d \n",WEXITSTATUS(status));
    }
}


int main(int argc, char *argv[])
{
    pid_t pid;
    struct sigaction act;
    
    act.sa_handler = read_childproc;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, 0);
    pid = fork();
    if(pid==0){
        puts("Hi，I'm child process");
        sleep(10);
        return 12;
    }else{
        printf("Child proc id : %d\n",pid);
        pid = fork();
        if(pid==0){
            puts("Hi, I'm child process");
            sleep(10);
            exit(24);
        }else{
            printf("Child proc id : %d\n",pid);
            for(int i=0; i<5; i++){
                puts("wait...");
                sleep(5);
            }
        }
    }
    return 0;
}
```

#### 编译运行

```shell
$ gcc remove_zombie.c -o ./bin/remove_zombie 
$ ./bin/remove_zombie 
Child proc id : 8657
Child proc id : 8658
wait...
Hi, I'm child process
Hi，I'm child process
wait...
Remove oroc id: 8658
Child send: 24 
wait...
Remove oroc id: 8657
Child send: 12 
wait...
wait...
```

当子进程结束时产生SIGCHLD信号，由操作系统代执行read_childproc函数来销毁子进程。



## 6、基于多任务的并发服务器

### 6.1 基于进程的并发服务器模型

为每一个请求连接的客户端提供一个进程提供服务，这样就可以同时为多个客户端提供服务。实现分三个阶段：

1. 回声服务器端（父进程）通过调用accept函数受理连接请求。
2. 此时获取的套接字文件描述符创建并传递给子进程
3. 子进程利用传递来的文件描述符提供服务

#### 示例代码

echo_mpserv.c

```c
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#define BUF_SIZE 1024
void errorHandling(const char *message);
void read_childproc(int sig);

int main(int argc, char *argv[])
{
    int str_len,state;
    int serv_sock, clnt_sock;
    char message[BUF_SIZE];

    pid_t pid;
    struct sigaction act;
    act.sa_handler = read_childproc;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    state = sigaction(SIGCHLD, &act, 0);


    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    if (argc != 2)
    {
        printf("Usage: %s <port> \n", argv[1]);
        exit(1);
    }

    //调用socket函数创建套接字
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
    {
        errorHandling("socket() error");
    }
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    //调用bind函数分配IP地址和端口号
    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
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

    while(1){

        //调用accept函数受理连接请求，如果在没有连接请求的情况下调用该函数，则不会返回，直到有连接请求为止。
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
        if (clnt_sock == -1)
        {
            continue;
        }
        else
        {
            printf("new connected client\n");
        }
        pid = fork();
        if(pid == -1){
            close(clnt_sock);
            continue;
        }
        if(pid==0){
            close(serv_sock);
            while ((str_len = read(clnt_sock, message, BUF_SIZE))){
                //str_len表示读取到的字符串长度
                write(clnt_sock, message, str_len);
            }

            close(clnt_sock);
            puts("client disconnected...");
            return 0;
        }else{
            close(clnt_sock);
        }
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

void read_childproc(int sig){
    pid_t pid;
    int status;
    pid = waitpid(-1, &status, WNOHANG); //-1代表可以等待任意子进程终止  WNOHANG即使没有终止的子进程也不会进入阻塞状态，而是返回0并退出函数
    printf("remove proc id: %d\n",pid);
}
```



#### 编译运行

```shell
$ gcc echo_mpserv.c -o ./bin/echo_mpserv
$ ./bin/echo_mpserv 9190
new connected client
client disconnected...
remove proc id: 20943
```

```shell
$ gcc echo_mpclient.c -o ./bin/echo_mpclient
$ ./bin/echo_mpclient 127.0.0.1 9190
Connected......
Input message(Q to quit):hello world
Message from server: hello world
Input message(Q to quit):how are you
Message from server: how are you
Input message(Q to quit):123
Message from server: 123
Input message(Q to quit):q
```



## 7 分割TCP的I/O程序

在上面的示例中，对于客户端：向服务器端传输数据，并等待服务器的回复。无条件等待，直到接受完服务器端的回声数据后，才能传输下一批数据。



### 7.1分割I/O程序的优点

即读写分离，分别用独立的两个进程来读取数据和发送数据。这样可以提高数据传输的效率。

* 在1个进程内同时实现数据收发逻辑需要考虑更多细节。程序越复杂，这种区别越明显，这也是公认的优点。
* 可以提高频繁交换数据的程序性能

#### 示例代码

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
void errorHandling(const char *message);
void read_routine(int sock, char *buf);
void write_routine(int sock, char *buf);
int itoc(int num, char *str);
int main(int argc, char *argv[])
{
    int sock;
    char message[BUF_SIZE];
    int str_len;
    struct sockaddr_in serv_adr;

    pid_t pid;


    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        errorHandling("socket() error");
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        errorHandling("connect() error!");
    }
    else
    {
        puts("Connected......");
    }

    pid = fork();
    if(pid==0){
        write_routine(sock,message);
    }else{
        read_routine(sock, message);
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

void read_routine(int sock, char *buf){
    while(1){
        int str_len = read(sock, buf, BUF_SIZE);
        if(str_len == 0)
            return;

        buf[str_len] = 0;
        printf("Message from server: %s",buf);
    }
}

void write_routine(int sock, char *buf){
    while(1){
        fgets(buf,BUF_SIZE, stdin);
        if (!strcmp(buf, "q\n") || !strcmp(buf, "Q\n")){
            shutdown(sock, SHUT_WR);
            return;
        }

        write(sock,buf,strlen(buf));
        
    }
}

int itoc(int num, char *str)
{
    char tem[1024];
    int id = 0, id2 = 0;
    while (num)
    {
        int t = num % 10;
        tem[id++] = t + '0';
        num /= 10;
    }
    str[id--] = '\0';
    while (id >= 0)
    {
        str[id2++] = tem[id--];
    }
    return 0;
}
```



#### 编译运行

```shell
$ gcc echo_mpclient.c -o ./bin/echo_mpclient 
$ ./bin/echo_mpclient 127.0.0.1 9190
Connected......
hello world
Message from server: hello world
hhh 
Message from server: hhh
aaa
Message from server: aaa
bbb
Message from server: bbb
q
```



## 8、习题（参考答案）

(1) 下列关于进程的说法错误的是：

a. 从操作系统的角度上说，进程是程序运行的单位
b. 进程根据创建方式建立父子关系
c. 进程可以包含其他进程，即一个进程的内存空间可以包含其他进程
d. 子进程可以创建其他子进程，而创建出来的子进程还可以创建其他子进程，但所有这些进程只与一个父进程建立父子关系。

> 答：
>
> c：每进程的都含有独立的内存空间
>
> d：子进程a创建的子进程b，a与b是父子关系，但a的父进程与b不是父子关系。



(2) 调用fork函数创建子进程，以下关于子进程的描述错误的是？

a. 父进程销毁时也会同时销毁子进程
b. 子进程是复制父进程所有资源创建出的进程
c. 父子进程共享全局变量
d. 通过 fork 函数创建的子进程将执行从开始到 fork 函数调用为止的代码。

> 答：
>
> c：父子进程分别含有全局变量
>
> d：子进程具有父进程的所有资源，包括所有代码，除了代码块if(pid==0){}else{}中else部分



(3) 创建子进程时将复制父进程的所有内容，此时的复制对象也包含套接字文件描述符。编写程序验证复制的文件描述符整数值是否与原文件描述符整数值相同。

```c
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char* argv[]){

    int sock;
    pid_t pid;
    sock = socket(PF_INET, SOCK_STREAM, 0);
    pid = fork();
    if(pid==0){
        printf("Child proc sock %d\n",sock);
    }else{
        printf("Parent proc sock %d\n",sock);
    }
    return 0;
}
```

编译运行

```shell
$ gcc test-sock.c -o ./bin/test-sock
$ ./bin/test-sock 
Parent proc sock 3
Child proc sock 3
```



(4)请说明进程变为僵尸进程的过程及预防措施？

进程变为僵尸进程往往是子进程已经执行结束，父进程没有及时销毁子进程，此时子进程就变成僵尸进程。

预防措施：父进程主动销毁子进程，可以通过wait函数，waitpid函数，还可以通过信号处理技术，委托操作系统在适当时候销毁子进程。



![截屏2021-05-07 上午12.53.42](https://cdn.jsdelivr.net/gh/wangjunstf/pics@main/uPic/%E6%88%AA%E5%B1%8F2021-05-07%20%E4%B8%8A%E5%8D%8812.53.42.png)

```c
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>


void keycontrol(int sig){
    char msg[10];
    if(sig==SIGINT){
        puts("退出请输入y");
        fgets(msg, sizeof(msg), stdin);
        if (!strcmp("y\n", msg) || !strcmp("Y\n", msg))
        {
            exit(0);
        }
    }
}

int main(int argc, char* argv[]){
    signal(SIGINT,keycontrol);

    while(1){
        puts("Hello world!");
        sleep(1);
    }
    return 0;
}
```



编译运行

```shell
$ gcc print.c -o ./bin/print
$ ./bin/print
Hello world!
Hello world!
Hello world!
Hello world!
^C退出请输入y
y
```

