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
