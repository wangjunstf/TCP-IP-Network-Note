#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SIZE 30

void error_handling(char *message);

int main(int argc, char*argv[]){
    int send_sock;
    struct sockaddr_in broad_adr;
    FILE *fp;
    char buf[BUF_SIZE];
    int so_brd = 1;
    if (argc != 3)
    {
        printf("Usage %s <GroupIP> <PORT>", argv[0]);
        exit(1);
    }

    send_sock = socket(PF_INET, SOCK_DGRAM, 0);
    memset(&broad_adr, 0, sizeof(broad_adr));
    broad_adr.sin_family = AF_INET;
    broad_adr.sin_addr.s_addr = inet_addr(argv[1]);
    broad_adr.sin_port = htons(atoi(argv[2]));

    setsockopt(send_sock, SOL_SOCKET, SO_BROADCAST, (void *)&so_brd, sizeof(so_brd));
    if ((fp = fopen("news.txt", "r")) == NULL)
        error_handling("fopen() error");
    // news.txt 最后一行需多一个换行，不然接收端无法正常输出
    while (1)
    {
        if (fgets(buf, BUF_SIZE - 1, fp) == NULL)
            break;
        
        sendto(send_sock, buf, strlen(buf), 0, (struct sockaddr *)&broad_adr, sizeof(broad_adr));
    }

    fclose(fp);
    close(send_sock);
    return 0;
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}