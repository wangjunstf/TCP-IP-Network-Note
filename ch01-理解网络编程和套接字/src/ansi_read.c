#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 100
void errorHandling(const char* message);

int main () {
   FILE *fp;
   char buf[BUF_SIZE];

   //以只读方式打开文件,该文件必须存在
   fp = fopen("data.txt", "r");
   if(fp==NULL){
        errorHandling("fopen error");      
   }

   if((int)fread(buf, 1, sizeof(buf),fp)==-1){
        errorHandling("fread error");
   }
   printf("%s\n", buf);
   fclose(fp);
   return(0);
}

void errorHandling(const char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
