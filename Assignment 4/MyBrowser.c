#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define __USE_XOPEN
#include <time.h>
#include <fcntl.h>
#include <poll.h>

// struct to destructure the response of GET
struct response_GET
{
    char *version;
    int status_code;
    char *status_message;
    time_t expires;
    char *cache_control;
    char *content_language;
    int content_length;
    char *content_type;
    time_t last_modified;
    char *body;
};

// struct to destructure the response of PUT
struct response_PUT
{
    char *version;
    int status_code;
    char *status_message;
};

// function to construct the header of GET request
void construct_header_GET(char *query_type, char *url, char **header, char *ip)
{
    *header = (char *)malloc(2048);

    sprintf(*header, "GET /%s HTTP/1.1\r\n", url);

    char *temp = (char *)malloc(1024);
    sprintf(temp, "Host: %s\r\n", ip);
    strcat(*header, temp);

    char date[100];
    time_t t = time(NULL);
    struct tm tm_ = *gmtime(&t);
    strftime(date, sizeof(date), "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", &tm_);
    strcat(*header, date);

    char file_format[10];
    memset(file_format, 0, 10);
    int i = strlen(url) - 1;
    while (url[i] != '.')
    {
        i--;
    }
    i++;

    strncpy(file_format, url + i, strlen(url) - i);

    if (strcmp(file_format, "html") == 0)
        sprintf(temp, "Accept: text/html\r\n");
    else if (strcmp(file_format, "jpg") == 0)
        sprintf(temp, "Accept: image/jpeg\r\n");
    else if (strcmp(file_format, "pdf") == 0)
        sprintf(temp, "Accept: application/pdf\r\n");
    else
        sprintf(temp, "Accept: text/*\r\n");

    strcat(*header, temp);

    sprintf(temp, "Version: HTTP/1.1\r\n");
    strcat(*header, temp);

    sprintf(temp, "Accept-Language: en-US,us\r\n");
    strcat(*header, temp);

    sprintf(temp, "Connection: close\r\n");
    strcat(*header, temp);

    char time_two_days_ago[150];
    t = time(NULL);
    t -= 60 * 60 * 24 * 2;
    tm_ = *gmtime(&t);
    strftime(time_two_days_ago, sizeof(time_two_days_ago), "If-Modified-Since: %a, %d %b %Y %H:%M:%S %Z", &tm_);

    strcat(*header, time_two_days_ago);
    strcat(*header, "\r\n\r\n");
}

// function to parse the response of GET request
void parse_response_GET(char *header, struct response_GET *res_GET)
{
    char *token = strtok(header, " ");
    res_GET->version = strdup(token);
    token = strtok(NULL, " ");
    res_GET->status_code = atoi(token);
    token = strtok(NULL, "\n");
    char *temp = strdup(token);
    temp[strlen(temp) - 1] = '\0';
    res_GET->status_message = strdup(temp);
    free(temp);

    char *key = strtok(NULL, ": ");
    char *value = strtok(NULL, "\n");

    while (key != NULL)
    {
        if (strcmp(key, "Expires") == 0)
        {
            struct tm gmtm;
            memset(&gmtm, 0, sizeof(gmtm));
            strptime(&value[1], "%A, %d %B %Y %H:%M:%S %Z", &gmtm);
            res_GET->expires = mktime(&gmtm);
        }
        else if (strcmp(key, "Cache-Control") == 0)
            res_GET->cache_control = strdup(&value[1]);
        else if (strcmp(key, "Content-Language") == 0)
            res_GET->content_language = strdup(&value[1]);
        else if (strcmp(key, "Content-Length") == 0)
        {
            char *temp = strdup(&value[1]);
            temp[strlen(temp) - 1] = '\0';
            res_GET->content_length = atoi(temp);
            free(temp);
        }
        else if (strcmp(key, "Content-Type") == 0)
        {
            char *temp = strdup(&value[1]);
            temp[strlen(temp) - 1] = '\0';
            res_GET->content_type = strdup(temp);
            free(temp);
        }
        else if (strcmp(key, "Last-Modified") == 0)
        {
            struct tm gmtm;
            memset(&gmtm, 0, sizeof(gmtm));
            char *temp = strdup(&value[1]);
            temp[strlen(temp) - 1] = '\0';
            strptime(temp, "%A, %d %B %Y %H:%M:%S %Z", &gmtm);
            res_GET->last_modified = mktime(&gmtm);
            free(temp);
        }

        key = strtok(NULL, ": ");
        value = strtok(NULL, "\n");
    }
}

// function to parse the response of PUT request
void parse_response_PUT(char *header, struct response_PUT *res_PUT)
{
    char *token = strtok(header, " ");
    res_PUT->version = strdup(token);
    token = strtok(NULL, " ");
    res_PUT->status_code = atoi(token);
    token = strtok(NULL, "\n");
    res_PUT->status_message = strdup(token);
}

// function to construct the header of PUT request
void construct_header_PUT(char *query_type, char *url, char *file_name, char **header, char *ip, int bytes)
{
    *header = (char *)malloc(2048);

    sprintf(*header, "PUT /%s HTTP/1.1\r\n", file_name);

    char *temp = (char *)malloc(1024);
    sprintf(temp, "Host: %s\r\n", ip);
    strcat(*header, temp);

    char date[100];
    time_t t = time(NULL);
    struct tm tm_ = *gmtime(&t);
    strftime(date, sizeof(date), "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", &tm_);
    strcat(*header, date);

    sprintf(temp, "Version: HTTP/1.1\r\n");
    strcat(*header, temp);

    sprintf(temp, "Connection: close\r\n");
    strcat(*header, temp);

    sprintf(temp, "Content-Language: en-us\r\n");
    strcat(*header, temp);

    sprintf(temp, "Content-Length: %d\r\n", bytes);
    strcat(*header, temp);

    char file_format[10];
    memset(file_format, 0, 10);
    int i = strlen(file_name) - 1;
    while (file_name[i] != '.')
    {
        i--;
    }
    i++;

    strncpy(file_format, file_name + i, strlen(file_name) - i);

    if (strcmp(file_format, "html") == 0)
        sprintf(temp, "Content-Type: text/html\r\n");
    else if (strcmp(file_format, "jpg") == 0)
        sprintf(temp, "Content-Type: image/jpeg\r\n");
    else if (strcmp(file_format, "pdf") == 0)
        sprintf(temp, "Content-Type: application/pdf\r\n");
    else
        sprintf(temp, "Content-Type: text/*\r\n");

    strcat(*header, temp);
}

// function to print appropiate messages based on status code
_Bool check_status_code(int status_code, char **status_msg)
{
    if (status_code == 200)
    {
        printf("Request has succeeded, with status message - %s\n", *status_msg);
        return 1;
    }
    else if (status_code == 404)
    {
        printf("Server cannot find the requested resource, status message recieved - %s\n", *status_msg);
        return 0;
    }
    else if (status_code == 400)
    {
        printf("Server cannot process the request due to a client error, status message recieved - %s\n", *status_msg);
        return 0;
    }
    else if (status_code == 403)
    {
        printf("Server understands the request but refuses to authorize it, status message recieved - %s\n", *status_msg);
        return 0;
    }
    else
    {
        printf("Unknown error, status code recieved - %d\n", status_code);
        return 0;
    }
}

// function to create a connection and return the sockfd
int create_connection(char *ip, char *port)
{
    int sockfd, newsockfd;        // Socket descriptors
    struct sockaddr_in serv_addr; // Server address structure

    /*
        Open a socket.
        If sockfd == -1, there was an error in the opening of the socket
    */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Cannot create socket\n");
        exit(0);
    }

    /*
        Set the internet family, internet address and port number
    */
    serv_addr.sin_family = AF_INET;
    // inet_aton("10.147.166.115", &serv_addr.sin_addr);
    inet_aton(ip, &serv_addr.sin_addr);
    serv_addr.sin_port = htons(atoi(port));

    /*
    Connect to the server
    If connect() returns -1, there was an error in the connection
    */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("Unable to connect to server\n");
        exit(0);
    }

    return sockfd;
}

// function to check the presence of blank line in sent/recieved messages
char *check(int len, char *res)
{
    for (int i = 0; i < len - 1; ++i)
    {
        if (res[i] == '\n' && res[i + 1] == '\r')
            return res + i;
    }
    return NULL;
}

// function to extract the header from the recieved messages, as they may contain some part of body in them
char *fetch_header(int socketid, int *n)
{
    // allocate memory for the string
    char *res = (char *)malloc(sizeof(char));
    *n = 0;
    int len = 0, size = 0; // len is the length of the string, size is the size of the allocated memory
    // while the string is not null terminated recieve data from the socket
    while (len == 0 || (len > 0 && check(len, res) == NULL))
    {
        // temp array to recieve data from the socket
        char temp[50];
        memset(temp, 0, sizeof(temp));
        // recieve data from the socket
        int len2 = recv(socketid, temp, 50, 0);
        int i;
        // copy the data from temp to res
        for (i = 0; i < len2; i++)
        {
            if (len + i == size)
            {
                if (size == 0)
                    size = 1;
                else
                    size *= 2;
                res = (char *)realloc(res, size);
            }
            res[len + i] = temp[i];
        }
        len += len2;
    }

    char *ptr = check(len, res);
    *n = ptr - res + 3;
    *n = len - *n;
    return res;
}

int main()
{
    char *prompt = strdup("MyOwnBrowser > "); // the prompt

    while (1)
    {
        printf("%s", prompt);
        char *user_request = NULL;
        size_t size = 0;

        getline(&user_request, &size, stdin);
        user_request[strlen(user_request) - 1] = '\0';

        if (strcmp(user_request, "QUIT") == 0)
            break;

        char *query_type = strtok(user_request, " ");
        if (query_type == NULL)
            continue;

        if (strcmp(query_type, "GET") == 0)
        {
            char *url = strtok(NULL, " ");
            if (url == NULL)
            {
                continue;
            }

            // get the http, ip, file, port from the user entered url
            char *http = strtok(url, "/");
            char *ip = strtok(NULL, "/");
            char *file = strtok(NULL, ":");
            char *port = strtok(NULL, "");

            int sockfd = create_connection(ip, port);

            char *header = NULL;
            construct_header_GET(query_type, file, &header, ip);
            printf("%s\n", header);
            send(sockfd, header, strlen(header), 0);
            free(header);

            struct response_GET res_GET;
            FILE *fp = fopen(file, "w");
            int n = 0;

            struct pollfd poll_set[1];   // Poll set, contains file descriptors to be polled
            poll_set[0].fd = sockfd;     // Set the socket file descriptor in the poll set
            poll_set[0].events = POLLIN; // Set the events to be polled

            int res = poll(poll_set, 1, 3 * 1000);

            if (res == 0)
            {
                printf("Connection Timed Out after 3 second!\n");
                close(sockfd);
                continue;
            }

            char *line = fetch_header(sockfd, &n);

            char *ptr = strstr(line, "\n\r\n");

            ptr++;
            ptr++;
            header = (char *)calloc((ptr - line + 2), sizeof(char));
            int i = 0;
            for (; i < ptr - line + 1; i++)
            {
                header[i] = line[i];
            }
            header[i] = '\0';
            printf("%s\n", header);
            parse_response_GET(header, &res_GET);

            ptr++;
            fwrite(ptr, 1, n, fp);

            int len = res_GET.content_length - n;
            char c;
            while (len > 0)
            {
                int sz = recv(sockfd, &c, 1, 0);
                len -= sz;

                fwrite(&c, 1, 1, fp);
                if (!sz)
                    break;
            }

            fclose(fp);

            _Bool response_ok = check_status_code(res_GET.status_code, &res_GET.status_message);
            if (!response_ok)
                continue;

            if (fork() == 0) // create a child process to open the corresponding application
            {
                close(sockfd);
                if (strcmp(res_GET.content_type, "text/html") == 0)
                {
                    execlp("firefox", "firefox", file, NULL);
                }
                else if (strcmp(res_GET.content_type, "text/*") == 0)
                {
                    execlp("gedit", "gedit", file, NULL);
                }
                else if (strcmp(res_GET.content_type, "application/pdf") == 0)
                {
                    execlp("firefox", "firefox", file, NULL);
                }
                else if (strcmp(res_GET.content_type, "image/jpeg") == 0)
                {
                    execlp("firefox", "firefox", file, NULL);
                }

                exit(0);
            }

            free(header);
            close(sockfd);
        }
        else if (strcmp(query_type, "PUT") == 0)
        {
            char *url = strtok(NULL, " ");
            if (url == NULL)
                continue;
            char *file_name = strtok(NULL, " ");
            if (file_name == NULL)
                continue;
            // get the http, ip, file, port from the user entered url
            char *http = strtok(url, "/");
            char *ip = strtok(NULL, "/");
            char *dir = strtok(NULL, ":");
            char *port = strtok(NULL, "");

            int sockfd = create_connection(ip, port);

            char *header = NULL;
            char *file_path = (char *)calloc(strlen(dir) + strlen(file_name) + 1, sizeof(char));
            sprintf(file_path, "%s/%s", dir, file_name);

            int bytes = 0;
            char c;
            int len;
            int fp = open(file_path, O_RDONLY);
            while ((len = read(fp, &c, 1)) > 0)
            {
                bytes += len;
            }

            close(fp);
            construct_header_PUT(query_type, file_path, file_name, &header, ip, bytes);
            int i = 0;
            while (i < strlen(header))
            {
                send(sockfd, &header[i], 1, 0);
                printf("%c", header[i]);
                i++;
            }
            char *temp = "\r\n";
            send(sockfd, temp, 2, 0);

            fp = open(file_path, O_RDONLY);
            while (1)
            {
                len = read(fp, &c, 1);
                send(sockfd, &c, 1, 0);
                if (len == 0)
                {
                    break;
                }
            }

            struct pollfd poll_set[1];   // Poll set, contains file descriptors to be polled
            poll_set[0].fd = sockfd;     // Set the socket file descriptor in the poll set
            poll_set[0].events = POLLIN; // Set the events to be polled

            int res = poll(poll_set, 1, 3 * 1000);

            if (res == 0)
            {
                printf("Connection Timed Out after 3 second!\n");
                close(sockfd);
                continue;
            }

            free(file_path);
            close(fp);
            free(header);

            struct response_PUT res_PUT;
            header = fetch_header(sockfd, &len);
            printf("%s\n", header);
            parse_response_PUT(header, &res_PUT);
            _Bool response_ok = check_status_code(res_PUT.status_code, &res_PUT.status_message);

            free(header);
        }
        else
        {
            continue;
        }
    }

    printf("Closing the connection...\n");

    free(prompt);
    return 0;
}