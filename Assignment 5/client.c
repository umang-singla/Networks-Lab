#include "mysocket.h"
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

int main()
{
    // Create socket
    int sockfd = my_socket(AF_INET, SOCK_MyTCP, 0);
    if (sockfd == -1)
    {
        perror("Unable to create socket\n");
        exit(0);
    }

    // Create client address
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &servaddr.sin_addr);
    servaddr.sin_port = htons(20000);

    // Connect the socket
    int connect_ret = my_connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (connect_ret == -1)
    {
        perror("Unable to connect to server\n");
        exit(0);
    }

    // Send and recieve call


    for (int i = 0; i < 20; i++)
    {
        char sbuff[100];
        for (int j = 0; j < 100; j++)
            sbuff[j] = 0;
        sprintf(sbuff, "My message %d", i);
        my_send(sockfd, sbuff, strlen(sbuff) + 1, 0);
    }

    for (int i = 0; i < 20; i++)
    {
        char rbuff[100];
        for (int j = 0; j < 100; j++)
            rbuff[j] = 0;
        my_recv(sockfd, rbuff, 100, 0);
        printf("%s\n", rbuff);
    }
    
    my_close(sockfd);

    return 0;
}