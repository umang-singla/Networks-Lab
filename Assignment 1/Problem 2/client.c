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

    size_t len = 10;
    char buf[100];
    char *buffer = NULL;
    size_t readChar;

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

    printf("Enter an Expression:\n");
    readChar = getline(&buffer, &len, stdin);

    if(readChar >=1 && buffer[readChar-1]=='\n') buffer[readChar-1] = '\0';

    int i=0;
    while (buffer[i] != '\0')
    {
        int j=0;
        while (j<100&&buffer[i]!='\0')
        {
            buf[j] = buffer[i];
            j++;
            i++;
        }

        while (j<100)
        {
            buf[j]='\0';
            j++;
        }
        
        send(socketid, buf, strlen(buf) + 1, 0);        
    }

    free(buffer);

    strcpy(buf,"-3  +(2+3)");

    for(int i=0; i < 100; i++) buf[i] = '\0';
	recv(socketid, buf, 100, 0);
	printf("%s\n", buf);

		
	close(socketid);
	return 0;
}