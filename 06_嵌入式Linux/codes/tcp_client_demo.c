/*
 * Teaching purpose:
 * - Create a TCP client with POSIX socket APIs.
 * - Connect to a server, send a short message and read a response.
 *
 * Usage:
 *   ./tcp_client_demo 127.0.0.1 9000
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    const char *serverIp;
    int port;
    int sockfd;
    struct sockaddr_in serverAddr;
    const char *message = "hello from embedded linux tcp client\n";
    char rx[256];
    int n;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <server-ip> <port>\n", argv[0]);
        return 1;
    }

    serverIp = argv[1];
    port = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
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

    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) != 0) {
        fprintf(stderr, "connect failed: %s\n", strerror(errno));
        close(sockfd);
        return 1;
    }

    if (send(sockfd, message, strlen(message), 0) < 0) {
        perror("send");
        close(sockfd);
        return 1;
    }

    n = (int)recv(sockfd, rx, sizeof(rx) - 1, 0);
    if (n < 0) {
        perror("recv");
        close(sockfd);
        return 1;
    }

    rx[n] = '\0';
    printf("RX: %s\n", rx);

    close(sockfd);
    return 0;
}

