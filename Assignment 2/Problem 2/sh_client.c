#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

int main(){
    int socketid;
	struct sockaddr_in	server_addr; // server address

    int i;
    char buf[100]; // buf array to recieve data from server

    if ((socketid = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

	// providing the server address
    server_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &server_addr.sin_addr);
	server_addr.sin_port	= htons(20000);

	// connecting to the server
    if ((connect(socketid, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

    char *res;
	// recienving the data from server
    for(i=0; i < 100; i++) buf[i] = '\0';
    res = read_line(socketid);

	// printing the data received from server
	printf("%s ", res);
    size_t len, readChar;

    // reading the username from client
    char *buffer;
    readChar = getline(&buffer,&len,stdin);
    if(readChar >=1 && buffer[readChar-1]=='\n') buffer[readChar-1] = '\0';

    send_line(buffer, socketid);
    free(res);

    res = read_line(socketid);
    if(strcmp(res, "NOT-FOUND") == 0){
        printf("%s\n",res);
        free(res);
    }else{
        printf("%s\n",res);
        free(res);
        // taking bash command as input until exit is entered
        while (1)
        {
            printf("Enter the bash command >>> ");
            readChar = getline(&buffer,&len,stdin);
            // sinding username to the server
            if(readChar >=1 && buffer[readChar-1]=='\n') buffer[readChar-1] = '\0';
            send_line(buffer, socketid);

            // recieving the result from the server
            char *res;
            res = read_line(socketid);

            // printing the result
            if(strcmp(res, "####") == 0) printf("Error in running command\n");
            else if(strcmp(res, "$$$$") == 0) printf("Invalid  command\n");
            else if(strcmp(res, "exit") == 0){
                printf("Exiting the shell\n");
                free(res);
                break;
            }
            else printf("%s\n",res); 
            free(res);  
        }
    }
		
	// closing the connection with server
	close(socketid);
	return 0;
}