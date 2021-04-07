#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<fcntl.h>
#define BUF_SIZE 100
void errorHandling(const char* message);

int main(){
    int fd;
    char buf[BUF_SIZE];

    //打开读取专用文件data.txt
    fd = open("data.txt", O_RDONLY);
    if(fd == -1){
        errorHandling("open() error!");
    }
    printf("file descriptor:%d \n",fd);

    //调用read函数向声明的buf数组保存读入的数据
    if(read(fd, buf, sizeof(buf))== -1){
        errorHandling("read() error!");
    }
    
    printf("file data: %s",buf); 
    close(fd);
    return 0;
}

void errorHandling(const char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
