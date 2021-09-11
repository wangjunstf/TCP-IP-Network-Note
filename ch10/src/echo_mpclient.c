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