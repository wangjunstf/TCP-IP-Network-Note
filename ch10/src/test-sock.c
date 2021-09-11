#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char* argv[]){

    int sock;
    pid_t pid;
    sock = socket(PF_INET, SOCK_STREAM, 0);
    pid = fork();
    if(pid==0){
        printf("Child proc sock %d\n",sock);
    }else{
        printf("Parent proc sock %d\n",sock);
    }
    return 0;
}