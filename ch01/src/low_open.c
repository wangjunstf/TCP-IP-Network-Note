#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>

void errorHandling(const char* message);

int main(){
    int fd;
    char buf[] = "Let's go!\n";

    fd = open("data.txt", O_CREAT|O_WRONLY|O_TRUNC);
    if(fd == -1){
        errorHandling("open() error!");
    }
    printf("file descriptor: %d \n", fd);

    if(write(fd, buf, sizeof(buf))==-1)
        errorHandling("write() error!");

    close(fd);
    return 0;
}

void errorHandling(const char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
