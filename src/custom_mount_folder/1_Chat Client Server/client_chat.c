/*
    Client Side
    Usage:
        ./a.out <IP_ADDRESS> <PORT>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h>

void error(const char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[]) {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <IP_ADDRESS> <PORT>\n", argv[0]);
        exit(0);
    }

    // socket()
    portno = atoi(argv[2]);     // atoi: convert string to integer
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    // connect()
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    // START main work
    printf("INFO: Enter \"bye\" without quotes to stop the chat\n\n");
    while (1) {
        bzero(buffer, 256);
        printf("You: ");
        fgets(buffer, 255, stdin);
        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0) error("ERROR writing to socket");
        if (strncmp("bye", buffer, 3) == 0) break;

        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);
        if (n < 0) error("ERROR reading from socket");
        printf("Server : %s", buffer);
        if (strncmp("bye", buffer, 3) == 0) break;
    }
    
    close(sockfd);
    return 0;
}
