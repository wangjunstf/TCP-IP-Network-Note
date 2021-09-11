#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define SMALL_BUF 100

void* request_handler(void* arg);
void send_data(FILE* fp, char* ct, char* file_name);
char* content_type(char* file);
void send_error(FILE* fp);
void error_handling(char* message);

int main(int argc, char* argv[]){
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_size;
    char buf[BUF_SIZE];
    pthread_t t_id;
    if(argc != 2){
        printf("Usage: %s <PORT>\n",argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));
    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
        error_handling("bind error");
    
    if(listen(serv_sock, 20)==-1)
        error_handling("listen() error");
    
    while(1){
        clnt_adr_size = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_size);
        printf("Connect Request : %s:%d\n", inet_ntoa(clnt_adr.sin_addr), ntohs(clnt_adr.sin_port));
        pthread_create(&t_id, NULL, request_handler, &clnt_sock);
        pthread_detach(t_id);
    }
    close(serv_sock);
    return 0;
}

void * request_handler(void* arg){
    int clnt_sock = *((int*)arg);
    char req_line[SMALL_BUF];
    FILE* clnt_read;
    FILE* clnt_write;

    char method[10];
    char ct[15];
    char file_name[30];
    clnt_read = fdopen(clnt_sock, "r");        // fdopen 将文件描述符转为FILE指针
    clnt_write = fdopen(dup(clnt_sock), "w");  // dup 为复制文件描述符，
    fgets(req_line, SMALL_BUF, clnt_read);     // fgets从指定流中读取一行
    if(strstr(req_line, "HTTP/")==NULL){
        send_error(clnt_write);
        fclose(clnt_read);
        fclose(clnt_write);
        return NULL;
    }
    strcpy(method, strtok(req_line, " /"));
    strcpy(file_name, strtok(NULL, " /"));
    strcpy(ct, content_type(file_name));
    if(strcmp(method, "GET")!=0){
        send_error(clnt_write);
        fclose(clnt_write);
        fclose(clnt_read);
        return NULL;
    }
    fclose(clnt_read);
    send_data(clnt_write, ct, file_name);
}

void send_data(FILE* fp, char* ct, char* file_name){
    char path[] = "./";
    char protocol[] = "HTTP/1.0 200 OK\r\n";
    char server[] = "Server: Linux Web Server \r\n";
    char cnt_len[] = "Content-length:2048\r\n";
    char cnt_type[SMALL_BUF];
    FILE * send_file;
    char buf[BUF_SIZE];
    sprintf(cnt_type, "Content-type:%s\r\n\r\n",ct);
    char *file_path = malloc(sizeof(char) * (strlen(file_name) + strlen(path) + 1));
    strcpy(file_path, path);
    strcat(file_path, file_name);

    send_file = fopen(file_path,"r");
    if(send_file==NULL){
        send_error(fp);
        return;
    }
    // 传输头信息
    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_len,fp);
    fputs(cnt_type, fp);

    // 传输请求数据
    while(fgets(buf, BUF_SIZE, send_file)!=NULL){
        fputs(buf, fp);
        fflush(fp);
    }
    fflush(fp);
    fclose(fp);
}

char * content_type(char* file){
    char extension[SMALL_BUF];
    char file_name[SMALL_BUF];
    strcpy(file_name, file);
    strtok(file_name, ".");
    strcpy(extension, strtok(NULL, "."));

    if(!strcmp(extension, "html") || strcmp(extension, "htm"))
        return "text/html";
    else
        return "text/plain";
}

void send_error(FILE *fp){
    char protocol[] = "HTTP/1.0 404 Not Found\r\n";
    char date[] = " Tue, 10 Jul 2012 06:50:15 GMT\r\n";
    char cnt_len[] = "Content-Length: 2048\r\n";
    char cnt_type[] = "Content-Type: text/html;charset=utf-8\r\n\r\n";

    char server[] = "Server: Linux Web Werver \r\n";
    char content[] = "<html><head><meta charset=\"utf-8\"><title>NETWORK</title></head>"
                                                          "<body><front size=+5><br>发生错误！查看请求文件名和请求方式！</front></body>"
                                                          "</html>";
    fputs(protocol, fp);
    fputs(date, fp);
    fputs(server, fp);
    fputs(cnt_len,fp);
    fputs(cnt_type, fp);
    fputs(content, fp);
    fflush(fp);
}

void error_handling(char* message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);

}