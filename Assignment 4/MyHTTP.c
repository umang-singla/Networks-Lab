#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h> 
#define _USE_XOPEN
#define GNU_SOURCE
#include <time.h>
#include <fcntl.h>
#define MAXLINE 1000

// function to check if the delimiter is there in the string or not
char *check(int len, char *res)
{
    for(int i=0; i<len-1; ++i)
    {
        if(res[i]=='\n' && res[i+1]=='\r')
            return res+i;
    }
    return NULL;
}

// function to receive a line from the socket
char* read_line(int socketid){
    // allocate memory for the string
    char *res = (char *)malloc(sizeof(char));
    int len = 0, size = 0; // len is the length of the string, size is the size of the allocated memory
    // while the string is not null terminated recieve data from the socket
    while (len == 0 || (len > 0 && check(len, res) == NULL))
    {
        // temp array to recieve data from the socket
        char temp[50];
        // recieve data from the socket
        int len2 = recv(socketid, temp, 50, 0);
        int i;
        // copy the data from temp to res
        for(i=0;i<len2;i++){
            if(len  + i == size){
                if(size == 0) size = 1;
                else size *= 2;
                res = (char *)realloc(res, size);
            }
            res[len+i] = temp[i];
        }
        len = len+len2;      
    }
    
    // return the string
    return res;
}

// function to fetch the headers of the request
char * fetch_header(int socketid,int *n){
     // allocate memory for the string
    char *res = (char *)malloc(sizeof(char));
    int len = 0, size = 0; // len is the length of the string, size is the size of the allocated memory
    // while the string is not null terminated recieve data from the socket
    while (len == 0 || (len > 0 && check(len, res) == NULL))
    {
        // temp array to recieve data from the socket
        char temp[50];
        // recieve data from the socket
        int len2 = recv(socketid, temp, 50, 0);
        int i;
        // copy the data from temp to res
        for(i=0;i<len2;i++){
            if(len  + i == size){
                if(size == 0) size = 1;
                else size *= 2;
                res = (char *)realloc(res, size);
            }
            res[len+i] = temp[i];
        }
        len = len+len2;      
    }

    char *ptr = strstr(res, "\n\r");
    *n = ptr - res + 2;
    *n = len - *n;
    *n -=1;
    
    // return the string
    return res;
}

// function to send a line to the socket
void send_line(char *line, int socketid){
    char buffer[50];
    int len = strlen(line);
    // send the data in chunks of 50 bytes
    for(int i=0;i<len;i++){
        buffer[i%50] = line[i];
        if(i%50 == 49) send(socketid, buffer, 50, 0); // sending data when the buffer is full
    }
    // sending the remaining data
    buffer[len%50] = '\0';
    while (len%50 != 0)
    {
        buffer[len%50] = '\0';
        len++;
    }
    send(socketid, buffer, strlen(buffer) + 1, 0);   

}

// structure to store the header of the request
struct http_header{
    char *method;
    char *path;
    char *version;
    time_t date;
    char *host;
    char *connection;
    char *accept_language;
    char *accept;
    time_t if_modified_since;
    char *content_language;
    char *content_type;
    int content_length;
};

void put_request(struct http_header *request_header, int sockid, struct sockaddr_in client_addr){
    int n=0;
    char *line = fetch_header(sockid,&n);

    // counting the number of fields in the header
    char prev = 'a';
    int i=0, cnt = 0;
    while (1)
    {
        if(prev == '\n' && line[i] == '\r'){
            printf("\n\r\n");
            break;
        }
        if(line[i] == '\n') cnt++;
        prev = line[i];
        printf("%c", line[i]);
        fflush(stdout);
        i++;
    }

    //parsing the http header adn storing them in the structure
    char *token = strtok(line, " ");
    request_header->path = &token[1];
    request_header->version = strtok(NULL, "\r");

    char *key = strtok(NULL, ": ");
    char *value = strtok(NULL, "\r");
    key = &key[1];

    cnt--;

    while (cnt--)
    {
        if(strcmp(key, "Host") == 0){
            request_header->host = strdup(&value[1]);
        }
        else if(strcmp(key, "Connection") == 0){
            request_header->connection = strdup(&value[1]);
        }
        else if(strcmp(key, "Accept-Language") == 0){
            request_header->accept_language = strdup(&value[1]);
        }
        else if(strcmp(key, "Accept") == 0){
            request_header->accept = strdup(&value[1]);
        }
        else if(strcmp(key, "If-Modified-Since") == 0){
            struct tm gmtm;
            strptime(&value[1], "%a, %d %b %Y %H:%M:%S %Z", &gmtm);
            request_header->if_modified_since = mktime(&gmtm);
        }
        else if(strcmp(key, "Content-Language") == 0){
            request_header->content_language = strdup(&value[1]);
        }
        else if(strcmp(key, "Content-Type") == 0){
            request_header->content_type = strdup(&value[1]);
        }
        else if(strcmp(key, "Content-Length") == 0 || strcmp(key, "content-length") == 0){
            request_header->content_length = atoi(&value[1]);
        }else if(strcmp(key, "Date") == 0){
            struct tm gmtm;
            memset(&gmtm, 0, sizeof(gmtm));
            if (strptime(&value[1], "%A, %d %B %Y %H:%M:%S %Z", &gmtm) == NULL) printf("error");
            request_header->date = mktime(&gmtm);
        }

        if(cnt == 0){
            strtok(NULL, "\n");
            break;
        }
        key = strtok(NULL, ": ");
        key = &key[1];
        value = strtok(NULL, "\r");
    }

    // updating the access log
    FILE *access_log = NULL;
    access_log = fopen("AccessLog.txt", "a");
    struct tm *gmtm = gmtime(&request_header->date);
    fprintf(access_log, "%02d%02d%02d:%02d%02d%02d:%s:%d:PUT:%s\n",gmtm->tm_mday, gmtm->tm_mon+1, (gmtm->tm_year)%100, gmtm->tm_hour, gmtm->tm_min, gmtm->tm_sec, inet_ntoa(client_addr.sin_addr), client_addr.sin_port, request_header->path);
    fclose(access_log);

    char *body = strtok(NULL, "");

    char c;

    // downloading the file
    FILE *fp = fopen(request_header->path, "wb");
    fwrite(body, 1, n, fp);
    int len = request_header->content_length - n;
    while (len>0)
    {
        len -= recv(sockid, &c, 1, 0);
        fwrite(&c, 1, 1, fp);
    }

    fclose(fp);

    // sending the http PUT request response
    printf("HTTP/1.1 200 OK\r\n\r\n");
    fflush(stdout);
    char *temp = strdup("HTTP/1.1 200 OK\r\n\r\n");
    int send_len=send(sockid, temp, strlen(temp) + 1, 0);
    fflush(stdout);
}

// function to handle get method
void get_request(struct http_header *request_header, int sockid, struct sockaddr_in client_addr){
    // function to read headers and store them in a struct
    char *line = read_line(sockid);
    printf("%s", line);
    fflush(stdout);

    // parsing the headers and storing them in the struct
    char *token = strtok(line, " ");
    request_header->path = &token[1];
    request_header->version = strtok(NULL, "\r\n");

    char *key = strtok(NULL, ": ");
    char *value = strtok(NULL, "\r\n");
    key = &key[1];
    int bad_request = 0; //flag to check if the request is bad or not
    
    // parsing the headers and storing them in the struct
    while (key != NULL)
    {
        if(strcmp(key, "Host") == 0){
            request_header->host = strdup(&value[1]);
        }
        else if(strcmp(key, "Connection") == 0){
            request_header->connection = strdup(&value[1]);
            if(strcmp(request_header->connection, "close") != 0){
                bad_request = 1;
            }
        }
        else if(strcmp(key, "Accept-Language") == 0){
            request_header->accept_language = strdup(&value[1]);
        }
        else if(strcmp(key, "Accept") == 0){
            request_header->accept = strdup(&value[1]);
        }
        else if(strcmp(key, "If-Modified-Since") == 0){
            struct tm gmtm;
            strptime(&value[1], "%a, %d %b %Y %H:%M:%S %Z", &gmtm);
            request_header->if_modified_since = mktime(&gmtm);
        }
        else if(strcmp(key, "Content-Language") == 0){
            request_header->content_language = strdup(&value[1]);
            if(strcmp(request_header->content_language, "en-US,us") != 0){
                bad_request = 1;
            }
        }
        else if(strcmp(key, "Content-Type") == 0){
            request_header->content_type = strdup(&value[1]);
        }
        else if(strcmp(key, "Content-Length") == 0 || strcmp(key, "content-length") == 0){
            request_header->content_length = atoi(&value[1]);
        }else if(strcmp(key, "Date") == 0){
            struct tm gmtm;
            memset(&gmtm, 0, sizeof(gmtm));
            if (strptime(&value[1], "%A, %d %B %Y %H:%M:%S %Z", &gmtm) == NULL){
                bad_request = 1;
            }
            request_header->date = mktime(&gmtm);
        }

        key = strtok(NULL, ": ");
        if(key != NULL) key = &key[1];
        value = strtok(NULL, "\r\n");
    }

    // getting the status of the file
    struct stat file_stat;
    stat(request_header->path, &file_stat);

    // updating the access log
    FILE *access_log = NULL;
    access_log = fopen("AccessLog.txt", "a");
    struct tm *gmtm = gmtime(&request_header->date);
    fprintf(access_log, "%02d%02d%02d:%02d%02d%02d:%s:%d:GET:%s\n",gmtm->tm_mday, gmtm->tm_mon+1, (gmtm->tm_year)%100, gmtm->tm_hour, gmtm->tm_min, gmtm->tm_sec, inet_ntoa(client_addr.sin_addr), client_addr.sin_port, request_header->path);
    fclose(access_log);

    // sending http headers if the request is bad
    if(bad_request == 1){
        char *response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        send_line(response, sockid);
        return;
    }

    // checking if the file exists or not
    if(access(request_header->path, F_OK) == -1){
        fflush(stdout);
        char *response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send_line(response, sockid);
        return;
    }else{
        int bytes = 0;
        char c;
        int len;

        // checking if the file is readable or not
        if(access(request_header->path, R_OK) == -1){
            char *response = "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n";
            send_line(response, sockid);
            return;
        }

        FILE *fp = NULL;
        fp = fopen(request_header->path, "rb");
        while((len = fread(&c, 1, 1, fp)) > 0){
            bytes += len;
        }
        fclose(fp);

        fp = fopen(request_header->path, "rb");

        char *response = (char *)malloc(1000);

        time_t now = time(NULL);
        now += 3*24*60*60;

        char expires[50];
        struct tm tm = *gmtime(&now);
        strftime(expires, sizeof(expires), "%a, %d %b %Y %H:%M:%S %Z", &tm);
        char last_modified[50];
        struct tm tm2 = *gmtime(&file_stat.st_mtime);
        strftime(last_modified, sizeof(last_modified), "%a, %d %b %Y %H:%M:%S %Z", &tm2);
        // sending the http get response header
        sprintf(response, "HTTP/1.1 200 OK\r\nExpires: %s\r\nCache-Control: no-store\r\nContent-Language: en-US,en\r\nContent-Length: %d\r\nContent-Type: %s\r\nLast-Modified: %s\r\n\r\n",expires, bytes, request_header->accept, last_modified);
        
        int i=0;
        while (i<strlen(response))
        {
            send(sockid, &response[i], 1, 0);
            printf("%c", response[i]);
            i++;
        }

        // sending the file
        while (fread(&c, 1, 1, fp) > 0)
        {
            send(sockid, &c, 1, 0);
        }
        fclose(fp);
        
        fflush(stdout);
    }
}

// function to handle http request
void handle_http_request(int sockid, struct sockaddr_in client_addr){

    // struct to store http headers
    struct http_header request_header;
    
    char request[20];
    int n = 0;
    // getting the method of request
    while (n < 3)
    {
        int len = recv(sockid, &request[n], 3-n, 0);
        n += len;
    }

    request[n] = '\0';

    request_header.method = strdup(request);

    printf("%s", request);
    fflush(stdout);

    // solving the request according to the method
    if(strcmp(request_header.method, "GET") == 0){
        get_request(&request_header, sockid, client_addr);
        fflush(stdout);
    }else{
        put_request(&request_header, sockid, client_addr);
    }
    
}

// main function begins
int main(int argc, char *argv[]){
    int socketid;
    struct sockaddr_in server_addr, client_addr;

    int i;
    char buffer[MAXLINE]; // buffer to store the date and time

    // creating a socket
    if ((socketid = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

    // providing the server address
    server_addr.sin_family		= AF_INET;
	server_addr.sin_addr.s_addr	= INADDR_ANY;
	server_addr.sin_port		= htons(atoi(argv[1]));

    // binding the socket to the server address
    if (bind(socketid, (struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

    // listening for connections
    listen(socketid, 5);
    printf("Listening for requests...\n");
    srand(time(NULL));

    // Iterative server that handles one client at a time
    for(;;){
        int clilen = sizeof(client_addr);
        // accepting a connection
        int new_socketid = accept(socketid, (struct sockaddr *) &client_addr, &clilen);

        if (new_socketid < 0) {
			perror("Accept error\n");
			exit(0);
		}

        // recieving the request from the client
        if(fork() == 0){
            handle_http_request(new_socketid, client_addr);

            close(new_socketid);
            exit(0);
        }
        
        // closing the connection with the client
		close(new_socketid);
    }

    return 0;
}
