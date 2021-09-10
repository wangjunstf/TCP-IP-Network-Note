#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>

void errorHandling(const char* message);

int main(int argc, char* argv[]){
    int sock = 0; 
    int str_len = 0;
    int idx=0,read_len = 0;
    struct sockaddr_in serv_addr;
    char buf[32];

    if(argc != 3){
        printf("Usage: %s <ip> <port> \n", argv[0]);
        exit(1);
    }

    //创建套接字，此时套接字并不马上分为服务器端和客户端。
    //如果紧接着调用bind,listen函数，将成为服务器端套接字；
    //如果调用connect函数，将成为客户端套接字
    //前两个参数为PF_INET,SOCK_STREAM，因而可以省略第三个参数，用0代替
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        errorHandling("socket() error");
    }


    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));
    
    //调用connect函数，向服务器端发送连接请求
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        errorHandling("connect() error!");
    
    }
    
    //反复调用read函数，每次读取1个字节，如果read返回0，则循环条件为假，跳出while循环
    while(read_len = read(sock,&buf[idx++],1)){
        if(read_len == -1){
            errorHandling("read() error!");
        }
        //执行该语句，read_len始终为1，str_len记录读取的字节数
        str_len+=read_len;
    }
    printf("Message from server: %s \n", buf);
    printf("Function read call count: %d \n",str_len);
    
    close(sock);

    return 0;
}

void errorHandling(const char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
