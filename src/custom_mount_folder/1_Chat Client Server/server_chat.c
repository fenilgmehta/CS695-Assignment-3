/*
    A simple server in the internet domain using TCP
    Usage: ./a.out <PORT>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port number provided\n");
        printf("Usage: %s <PORT>\n", argv[0]);
        exit(1);
    }

    // socket()
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    // bind()
    portno = atoi(argv[1]);
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    // listen()
    listen(sockfd, 5);

    // accept()
    int clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) error("ERROR on accept");

    // START main work
    printf("INFO: Enter \"bye\" without quotes to stop the chat\n\n");
    while (1) {
        bzero(buffer, 256);
        n = read(newsockfd, buffer, 256);
        if (n < 0) error("ERROR reading from socket");
        printf("Client: %s", buffer);
        if (strncmp("bye", buffer, 3) == 0) break;

        bzero(buffer, 256);
        printf("You: ");
        fgets(buffer, 256, stdin);
        n = write(newsockfd, buffer, strlen(buffer));
        if (n < 0) error("ERROR writing to socket");
        if (strncmp("bye", buffer, 3) == 0) break;
    }

    close(newsockfd);
    close(sockfd);
    return 0;
}
