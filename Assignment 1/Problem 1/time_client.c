#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

	// recienving the data from server
    for(i=0; i < 100; i++) buf[i] = '\0';
	recv(socketid, buf, 100, 0);

	// printing the data received from server
	printf("%s\n", buf);
		
	// closing the connection with server
	close(socketid);
	return 0;
}