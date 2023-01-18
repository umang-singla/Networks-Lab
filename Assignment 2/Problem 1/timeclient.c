// A Simple Client Implementation
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <poll.h>

#define MAX_LINE 1024
  
int main() { 
    int sockfd; 
    struct sockaddr_in servaddr; 
  
    // Creating socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(8181); 
    inet_aton("127.0.0.1", &servaddr.sin_addr); 

    char buffer[MAX_LINE];
      
    int n;
    socklen_t len; 
    char *hello = "CLIENT:HELLO"; 

    int cnt = 5, done = 0;
    while (cnt--&& !done)
    {
        sendto(sockfd, (const char *)hello, strlen(hello), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 
        len = sizeof(servaddr);
        struct pollfd fds;
        fds.fd = sockfd;
        fds.events = POLLIN;
        int ret = poll(&fds, 1, 3000);  
        if (ret == 0)
        {
            if(cnt==0) printf("Timeout\n");
            continue;
        }
        else if (ret < 0)
        {
            perror("poll");
            exit(1);
        }      
        recvfrom(sockfd, (char *)buffer, MAX_LINE, 0, (struct sockaddr *) &servaddr, &len);
        done = 1;
        printf("%s", buffer);
    }
    
           
    close(sockfd); 
    return 0; 
} 