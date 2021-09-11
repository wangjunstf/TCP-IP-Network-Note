#include <stdio.h>
#include <string.h>
#define BUF_SIZE 50
int main(){
    FILE *fp = fopen("data.txt","wt");
    char buf[BUF_SIZE] = "hello world, how are you?";
    fwrite((void*)buf, 1, strlen(buf), fp);
    return 0;
}