#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

int main(){
    int socketid;
    struct sockaddr_in server_addr, client_addr;

    int i;
    char buffer[100];

    if ((socketid = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

    server_addr.sin_family		= AF_INET;
	server_addr.sin_addr.s_addr	= INADDR_ANY;
	server_addr.sin_port		= htons(20000);

    if (bind(socketid, (struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

    listen(socketid, 5);

    for(;;){
        int clilen = sizeof(client_addr);
        int new_socketid = accept(socketid, (struct sockaddr *) &client_addr, &clilen);

        if (new_socketid < 0) {
			perror("Accept error\n");
			exit(0);
		}

        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        sprintf(buffer,"Current Date & Time: %d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		send(new_socketid, buffer, strlen(buffer) + 1, 0);

        recv(new_socketid, buffer, 100, 0);
		printf("%s\n", buffer);

		close(new_socketid);
    }

    return 0;
}