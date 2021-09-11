#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30

void error_handling(char *message);

int main(int argc, char* argv[]){
    int sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t adr_len;

    struct sockaddr_in serv_adr, from_adr;

    if(argc!=3){
        printf("Usage: %s <IP> <PORT>\n",argv[1]);
        exit(1);
    }

    sock = socket(AF_INET, SOCK_DGRAM,0);
    if(sock==-1){
        error_handling("socket() error");
    }

    memset(&serv_adr, 0 , sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[2]));

    while(1){
        fputs("Insert message(q to quit)",stdout);
        fgets(message,sizeof(message),stdin);

        if(!strcmp("q\n",message)||!strcmp("Q!\n",message)){
            break;
        }

        sendto(sock, message, strlen(message),0, (struct sockaddr*)&serv_adr, sizeof(serv_adr));

        adr_len = sizeof(from_adr);
        str_len = recvfrom(sock, message, BUF_SIZE, 0, (struct sockaddr*)&from_adr, &adr_len);

        message[str_len] = 0;
        printf("Message from server:%s",message);
    }

    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
