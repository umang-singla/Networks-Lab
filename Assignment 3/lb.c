#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h> 
#include <poll.h>
#include <time.h>

// function to receive a line from the socket
char* read_line(int socketid){
    // allocate memory for the string
    char *res = (char *)malloc(sizeof(char));
    int len = 0, size = 0; // len is the length of the string, size is the size of the allocated memory
    // while the string is not null terminated recieve data from the socket
    while (len == 0 || (len > 0 && res[len-1] != '\0'))
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



int main(int argc, char *argv[]){
    int socketid;
    struct sockaddr_in server_addr, client_addr;

    int i;
    char buffer[50000]; // buffer to store the date and time

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

    int load_s1=0, load_s2=0; // load_s1 is the load of server 1 and load_s2 is the load of server 2

    // Iterative server that handles one client at a time
    int tim = 5000; // timeout for poll
    for(;;){
        int clilen = sizeof(client_addr);
        // accepting a connection

        // time before calling poll
        time_t start = time(NULL);

        // calling poll
        struct pollfd fds;
        fds.fd = socketid;
        fds.events = POLLIN;
        int ret = poll(&fds, 1, tim);  

        // time after calling poll
        time_t end = time(NULL);
        int time_diff = end - start;
        tim = tim - time_diff*1000;
        // ret is 0 if timeout, -1 if error and >0 if data is available
        if(ret == 0){
            tim = 5000;
            int socket_s1, socket_s2;
            // creating a socket
            if ((socket_s1 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("Cannot create socket\n");
                exit(0);
            }


            // providing the server address
            server_addr.sin_family	= AF_INET;
            inet_aton("127.0.0.1", &server_addr.sin_addr);
            server_addr.sin_port	= htons(atoi(argv[2]));                    
            // connecting to the server1
            if ((connect(socket_s1, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0) {
                perror("Unable to connect to server\n");
                exit(0);
            }

            send_line("Send Load", socket_s1); // asking the load of server 1
            char *res = read_line(socket_s1); // recieving the load of server 1
            load_s1 = atoi(res);
            printf("Load recieved from 127.0.0.1:%s %d\n", argv[2], load_s1);
            close(socket_s1); // closing the connection with the server 1

            // creating a socket
            if ((socket_s2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("Cannot create socket\n");
                exit(0);
            }

            server_addr.sin_port	= htons(atoi(argv[3]));
            // connecting to the server 2
            if ((connect(socket_s2, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0) {
                perror("Unable to connect to server\n");
                exit(0);
            }

            send_line("Send Load", socket_s2); // asking the load of server 2
            res = read_line(socket_s2); // recieving the load of server 2
            load_s2 = atoi(res);
            printf("Load recieved from 127.0.0.1:%s %d\n", argv[3], load_s2);
            fflush(stdout);
            close(socket_s2); // closing the connection with the server
        }
        else if (ret < 0)
        {
            perror("poll");
            exit(1);
        }else{

            // accepting a connection
            int new_socketid = accept(socketid, (struct sockaddr *) &client_addr, &clilen);
            // checking if the connection is accepted
            if (new_socketid < 0) {
                perror("Accept error\n");
                exit(0);
            }

            // creating a child process
            if(fork() == 0){
                // providing the server address
                server_addr.sin_family	= AF_INET;
                inet_aton("127.0.0.1", &server_addr.sin_addr);
                int server_socketid;
                int port;

                // creating a socket to connect to the server with the least load
                if ((server_socketid = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    perror("Cannot create socket\n");
                    exit(0);
                }
                if(load_s1 <= load_s2){
                    server_addr.sin_port	= htons(atoi(argv[2]));  
                    port = atoi(argv[2]);                  
                }else{
                    server_addr.sin_port	= htons(atoi(argv[3]));
                    port = atoi(argv[3]);
                }
                // connecting to the server
                if ((connect(server_socketid, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0) {
                    perror("Unable to connect to server\n");
                    exit(0);
                }

                send_line("Send Time", server_socketid); // asking the server for time
                printf("Sending client request to 127.0.0.1:%d\n",port);
                fflush(stdout);
                char *res = read_line(server_socketid); // recieving the date and time from the server
                close(server_socketid); // closing the connection with the server
                send_line(res, new_socketid); // sending the date and time to the client
                // closing the connection with the client
                close(new_socketid);
                // exiting the process
                exit(0);
            }
            // closing the connection with the client
            close(new_socketid);
        }

    }

    return 0;
}