#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h> 
#include <time.h>
#define MAXLINE 1000

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

        char *res;
        res = read_line(new_socketid); // recieving the request of load balancer
        if(strcmp(res, "Send Load") == 0){
            // generating a random load
            int temp = rand();
            temp = 1 + temp%100;
            char load[10];

            sprintf(load, "%d", temp);
            send_line(load, new_socketid); // sending the load to the client
            printf("Load sent: %s\n", load);
            fflush(stdout);
        }else if(strcmp(res, "Send Time") == 0){
            // fetching the current date and time
            time_t t;
            time (&t);
            memset(buffer, '\0', MAXLINE);
            // sending the current date and time to the load balancer
            sprintf(buffer, "Current Date & Time: %s", ctime(&t));
            send_line(buffer, new_socketid);
        }

        // closing the connection with the client
		close(new_socketid);
    }

    return 0;
}