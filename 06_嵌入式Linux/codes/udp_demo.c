/*
 * Teaching purpose:
 * - Send one UDP datagram with POSIX socket APIs.
 * - Wait for a response with select timeout.
 *
 * Usage:
 *   ./udp_demo 127.0.0.1 9001
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    const char *serverIp;
    int port;
    int sockfd;
    struct sockaddr_in serverAddr;
    socklen_t addrLen;
    const char *message = "hello from embedded linux udp demo";
    char rx[256];
    fd_set readSet;
    struct timeval timeout;
    int ret;
    int n;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <server-ip> <port>\n", argv[0]);
        return 1;
    }

    serverIp = argv[1];
    port = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, serverIp, &serverAddr.sin_addr) != 1) {
        fprintf(stderr, "invalid ip: %s\n", serverIp);
        close(sockfd);
        return 1;
    }

    if (sendto(sockfd, message, strlen(message), 0,
               (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("sendto");
        close(sockfd);
        return 1;
    }

    FD_ZERO(&readSet);
    FD_SET(sockfd, &readSet);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    ret = select(sockfd + 1, &readSet, NULL, NULL, &timeout);
    if (ret < 0) {
        perror("select");
        close(sockfd);
        return 1;
    }

    if (ret == 0) {
        printf("UDP receive timeout.\n");
        close(sockfd);
        return 0;
    }

    addrLen = sizeof(serverAddr);
    n = (int)recvfrom(sockfd, rx, sizeof(rx) - 1, 0,
                      (struct sockaddr *)&serverAddr, &addrLen);
    if (n < 0) {
        perror("recvfrom");
        close(sockfd);
        return 1;
    }

    rx[n] = '\0';
    printf("RX: %s\n", rx);

    close(sockfd);
    return 0;
}

