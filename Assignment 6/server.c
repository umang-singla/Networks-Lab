#include <arpa/inet.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{

    int sockfd;
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket creation failed");
        exit(1);
    }

    // receiver address
    struct sockaddr_in recv_addr;
    socklen_t recv_addr_len = sizeof(recv_addr);
    memset(&recv_addr, 0, sizeof(recv_addr));

    // receive the packet
    char recv_packet[1024];
    int n = recvfrom(sockfd, recv_packet, sizeof(recv_packet), 0, (struct sockaddr *)&recv_addr, &recv_addr_len);
    if (n < 0)
    {
        perror("recvfrom failed");
        exit(1);
    }

    // extract the IP header
    struct iphdr *recv_ip = (struct iphdr *)recv_packet;

    // extract the ICMP header
    struct icmphdr *recv_icmp = (struct icmphdr *)(recv_packet + sizeof(struct iphdr));

    // check if the recv_packet is an ICMP time exceeded
    if (recv_icmp->type == ICMP_TIME_EXCEEDED)
    {
        printf("ICMP time exceeded received from %s\n", inet_ntoa(recv_addr.sin_addr));
    }

    // check if the recv_packet is an ICMP destination unreachable
    if (recv_icmp->type == ICMP_DEST_UNREACH)
    {
        printf("ICMP destination unreachable received from %s\n", inet_ntoa(recv_addr.sin_addr));
    }
}