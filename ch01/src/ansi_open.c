#include<stdio.h>
#include <stdlib.h>

void errorHandling(const char* message);

int main () {
   FILE *fp;
   
   char buf[] = "Let's go!\n";

   fp = fopen( "data.txt" , "w" );
   if((int)fwrite(buf, 1 , sizeof(buf) , fp )==-1){
        errorHandling("fopen erro");
   }

   fclose(fp);
  
   return(0);
}

void errorHandling(const char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
