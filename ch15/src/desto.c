#include <stdio.h>
#include <fcntl.h>

int main(int argc, char* argv[]){
    FILE *fp;
    // S_IRWXU 给文件所属用户赋予读写执行权限
    int fd = open("data.dat", O_WRONLY | O_CREAT | O_TRUNC,S_IRWXU);
    if(fd==-1){
        fputs("file open error",stdout);
        return -1;
    }

    fp=fdopen(fd,"w");
    fputs("Network c programming \n", fp);
    fclose(fp);
    return 0;
}