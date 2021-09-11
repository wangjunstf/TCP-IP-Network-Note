#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>


void keycontrol(int sig){
    char msg[10];
    if(sig==SIGINT){
        puts("退出请输入y");
        fgets(msg, sizeof(msg), stdin);
        if (!strcmp("y\n", msg) || !strcmp("Y\n", msg))
        {
            exit(0);
        }
    }
}

int main(int argc, char* argv[]){
    signal(SIGINT,keycontrol);

    while(1){
        puts("Hello world!");
        sleep(1);
    }
    return 0;
}