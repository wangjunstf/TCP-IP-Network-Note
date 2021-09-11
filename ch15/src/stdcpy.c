#include <stdio.h>
#include <time.h>
#define BUF_SIZE 3 //用最短长度构成

int main(int argc, char* argv[]){
    FILE *fp1;
    FILE *fp2;
    time_t start,end;
    double duration;

    char buf[BUF_SIZE];

    fp1 = fopen("news.txt","r");
    fp2 = fopen("cpy.txt", "w");
    
    start = clock();
    while(fgets(buf,BUF_SIZE, fp1)!=NULL)
        fputs(buf,fp2);
    end = clock();

    duration = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Running time: %f 秒\n",duration);

    fclose(fp1);
    fclose(fp2);
    return 0;
}