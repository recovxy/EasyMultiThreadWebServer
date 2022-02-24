#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUF_SIZE 1024
#define SMALL_BUF 100
#define SA struct sockaddr

void* request_handler(void* arg);
void send_data(FILE *fp, char* ct, char* file_name);
char* content_type(char* file);
void send_error(FILE *fp);
void error_handling(char* message);

int main(int argc, char* argv[])
{   
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    char buf[BUF_SIZE];
    pthread_t t_id;

    if(argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ( (server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        error_handling("socket() error");
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    if(bind(server_sock, (SA*)&server_addr, sizeof(server_addr)) == -1) {
        error_handling("bind() error");
    }
    if( listen(server_sock, 20) == -1) {
        error_handling("listen() error");
    }

    while (1)
    {
        client_addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (SA*)&client_addr, &client_addr_size);        
        printf("Connection Request: %s:%d\n",
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)
        );
        
        pthread_create(&t_id, NULL, request_handler, &client_sock);
        pthread_detach(t_id);
    }
    close(server_sock);
    return 0;
}

void* request_handler(void* arg)
{
    int client_sock = *((int*)arg);
    char req_line[SMALL_BUF];
    FILE* client_read;
    FILE* client_write;

    char method[10];
    char ct[15];
    char file_name[30];

    client_read = fdopen(client_sock, "r");
    client_write = fdopen(dup(client_sock), "w");
    fgets(req_line, SMALL_BUF, client_read);
    if(strstr(req_line, "HTTP/") == NULL){
        send_error(client_write);
        fclose(client_read);
        fclose(client_write);
        return NULL;
    }

    strcpy(method, strtok(req_line, " /"));
    strcpy(file_name, strtok(NULL, " /"));
    strcpy(ct, content_type(file_name));
    if(strcmp(method, "GET") != 0){
        send_error(client_write);
        fclose(client_read);
        fclose(client_write);
        return NULL;
    }
    fclose(client_read);
    send_data(client_write, ct, file_name);
    return NULL;
}

void send_data(FILE *fp, char* ct, char* file_name)
{
    char protocol[] = "HTTP/1.0 200 OK\t\n";
    char server[] = "Server:Linux Web Server \r\n";
    char cnt_len[] = "Content-length:2048\r\n";
    char cnt_type[SMALL_BUF];
    char buf[BUF_SIZE]; 
    FILE* send_file;

    sprintf(cnt_type, "Content-type:%s\r\n\r\n", ct);
    send_file=fopen(file_name, "r");
    if(send_file == NULL){
        send_error(fp);
        return;
    }

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_len, fp);
    fputs(cnt_type, fp);

    while (fgets(buf, BUF_SIZE, send_file) != NULL) 
    {
        fputs(buf, fp);
        fflush(fp);
    }

    fflush(fp);
    fclose(fp);
}

char* content_type(char* file)
{
    char extension[SMALL_BUF];
    char file_name[SMALL_BUF];
    strcpy(file_name, file);
    strtok(file_name, ".");
    strcpy(extension, strtok(NULL, "."));

    if(!strcmp(extension, "html") || !strcmp(extension, "htm")){
        return "text/html";
    }else{
        return "text/plain";
    }
}

void send_error(FILE *fp)
{
    char protocol[] = "HTTP/1.0 400 Bad Request\t\n";
    char server[] = "Server:Linux Web Server \r\n";
    char cnt_len[] = "Content-length:2048\r\n";
    char cnt_type[] = "Conetent-type:text/html\r\n\r\n";
    char content[] = "<html><head><title>NETWORK</title></head>"
        "<body><font size=+5><br>发生错误！请查看请求文件名和请求方式！"
        "</font></body></html>";

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_len, fp);
    fputs(cnt_type, fp);
    fflush(fp);
}


void error_handling(char* message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}