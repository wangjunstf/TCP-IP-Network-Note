#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
    // #include <stdlib.h>
    // #include <sys/wait.h>
    int main()
{
    pid_t pid;

    pid = fork();
    if(pid==0){
        printf("子进程执行区域\n");
        exit(0);
    }else{
        printf("父进程执行区域\n");
        sleep(12);
    }
    printf("子进程和父进程都将执行该代码\n");
    return 0;
}