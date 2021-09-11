#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#define BUF_SIZE 3  // 用最短数组长度构成

int main(int argc, char* argv[]){
    int fd1, fd2;   //保存在fd1和fd2中的是文件描述符
    int len;
    char buf[BUF_SIZE];
    clock_t start,end;
    double duration;   // 计算执行时间

    fd1 = open("news.txt", O_RDONLY);
    fd2 = open("cpy.txt", O_WRONLY | O_CREAT | O_TRUNC);
    start = clock();
    while((len=read(fd1,buf,sizeof(buf)))>0){
        write(fd2,buf,len);
    }
    end=clock();
    duration = (double)(end - start) / CLOCKS_PER_SEC;
    printf("running time: %f seconds\n", duration);

    close(fd1);
    close(fd2);
    return 0;
}