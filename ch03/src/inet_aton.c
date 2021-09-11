#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

void errorHandling(const char *message);

int main(int argc, char* argv[]){
    char* addr = "127.232.124.79";
    struct sockaddr_in addr_inet;

    if(!inet_aton(addr,&addr_inet.sin_addr)){
        errorHandling("Conversion error");
    }else{
        printf("Network ordered integer addr: %#x\n" ,addr_inet.sin_addr.s_addr);
    }
    return 0;
}
void errorHandling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}