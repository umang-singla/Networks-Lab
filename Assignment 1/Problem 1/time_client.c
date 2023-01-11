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
	struct sockaddr_in	server_addr;

    int i;
    char buf[100];

    if ((socketid = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

    server_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &server_addr.sin_addr);
	server_addr.sin_port	= htons(20000);

    if ((connect(socketid, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

    for(i=0; i < 100; i++) buf[i] = '\0';
	recv(socketid, buf, 100, 0);
	printf("%s\n", buf);

	strcpy(buf,"Message from client");
	send(socketid, buf, strlen(buf) + 1, 0);
		
	close(socketid);
	return 0;
}