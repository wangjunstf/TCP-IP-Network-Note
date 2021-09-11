#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 20
void error_handling(char *message);

int main(int argc, char *argv[])
{
    int serv_sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t clnt_sock_len;

    struct sockaddr_in serv_adr, cln_adr;

    if (argc != 2)
    {
        printf("Usage:%s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (serv_sock == -1)
    {
        error_handling("UDP creation is error");
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        error_handling("bind() error");
    }

    while (1)
    {
        clnt_sock_len = sizeof(cln_adr);
        str_len = recvfrom(serv_sock, message, BUF_SIZE, 0, (struct sockaddr *)&serv_adr, &clnt_sock_len);
        message[str_len]=0;
        if(str_len!=0){
            printf("Message from server:%s", message);
        }

        fputs("Insert message(q to quit)", stdout);
        fgets(message, sizeof(message), stdin);

        if (!strcmp("q\n", message) || !strcmp("Q!\n", message))
        {
            break;
        }

        sendto(serv_sock, message, str_len, 0, (struct sockaddr *)&serv_adr, clnt_sock_len);
    }

    close(serv_sock);

    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
