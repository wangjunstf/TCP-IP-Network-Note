#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#define BUF_SIZE 30

int main(int argc, char* argv[]){
    int fds[2];
    char buf[BUF_SIZE];
    char str1[] = "hello I am client.";
    char str2[] = "Nice to meet you.";
    char str3[] = "Nice to meet you too.";
    pid_t pid;
    int len;

    pipe(fds);
    pid = fork();
    if(pid==0){
        write(fds[1],str1,strlen(str1));
        sleep(2);
        len = read(fds[0], buf, BUF_SIZE);
        buf[len] = 0;
        printf("Child proc output: %s \n", buf);
        write(fds[1], str3, strlen(str3));
    }else{
        len = read(fds[0],buf,BUF_SIZE);
        buf[len] = 0;
        printf("Parent proc output: %s \n",buf);
        write(fds[1],str2,strlen(str2));
        sleep(3);
        len = read(fds[0], buf, BUF_SIZE);
        buf[len] = 0;
        printf("Parent proc output: %s \n", buf);
    }
    return 0;
}