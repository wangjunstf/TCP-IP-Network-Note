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