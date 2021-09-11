#include <stdio.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]){
    unsigned short host_port = 0x1234;
    unsigned short net_port;

    unsigned long host_addr = 0x12345678;
    unsigned long net_addr;

    //将net_port,net_addr转化为网络字节序
    net_port = htons(host_port); 
    net_addr = htonl(host_addr);

    printf("主机字节序端口号：%#x\n",host_port);
    printf("网络字节序端口号：%#x\n",net_port);

    printf("主机字节序地址：%#lx\n",host_addr);
    printf("网络字节序地址：%#lx\n",net_addr);
    return 0;
}