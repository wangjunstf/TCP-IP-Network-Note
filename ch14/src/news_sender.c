#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define TTL 64
#define BUF_SIZE 70

void error_handling(char *message);
int main(int argc, char*argv[]){
    int send_sock;
    struct sockaddr_in mul_adr;
    int time_live = TTL;
    int str_len;
    FILE *fp;
    char buf[BUF_SIZE];
    if (argc != 3)
    {
        printf("Usage %s <GroupIP> <PORT>", argv[0]);
        exit(1);
    }

    send_sock = socket(PF_INET, SOCK_DGRAM, 0);
    memset(&mul_adr, 0, sizeof(mul_adr));
    mul_adr.sin_family=AF_INET;
    mul_adr.sin_addr.s_addr = inet_addr(argv[1]);
    mul_adr.sin_port = htons(atoi(argv[2]));

    setsockopt(send_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&time_live, sizeof(time_live));
    if((fp=fopen("news.txt","r")) == NULL )
        error_handling("fopen() error");
    // news.txt 最后一行需多一个换行，不然接收端无法正常输出
    while(1){
        if(fgets(buf,BUF_SIZE-1, fp) == NULL){
            break;
        }
        sendto(send_sock, buf, strlen(buf), 0, (struct sockaddr*)&mul_adr, sizeof(mul_adr));
    }


    fclose(fp);
    close(send_sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}