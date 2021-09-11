#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[]){
    __pid_t pid = fork();
    if(pid==0){
        puts("Hi, I am a child process");
    }else{
        printf("Child Process ID : %d \n",pid);
        sleep(30);   // sleep 30 sec
    }

    if(pid==0){
        puts("End child process");
    }else{
        puts("END parent process");
    }
    return 0;
}