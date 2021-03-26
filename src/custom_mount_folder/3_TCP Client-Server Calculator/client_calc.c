/*
	Client Side
    Usage: ./a.out <HOSTNAME> <PORT>
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>

void error(const char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <HOSTNAME> <PORT>\n", argv[0]);
        exit(0);
    }

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[1024];

    // socket()
    portno = atoi(argv[2]);
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
    else
        printf("Successfully connected to the server :)\n");

    // START main work
    int num1;
    int num2;
    int ans;
    int choice;

    while(1){
        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);              //Read Server String
        if (n < 0)
            error("ERROR reading from socket");
        printf("\nSERVER: %s", buffer);
        scanf("%d", &choice);                       //Enter choice
        write(sockfd, &choice, sizeof(int));        //Send choice to Server

        if (choice == 5) break;

        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);              //Read Server String
        if (n < 0)
            error("ERROR reading from socket");
        printf("SERVER: %s", buffer);
        scanf("%d", &num1);                         //Enter No 1
        write(sockfd, &num1, sizeof(int));          //Send No 1 to Server

        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);              //Read Server String
        if (n < 0)
            error("ERROR reading from socket");
        printf("SERVER: %s", buffer);
        scanf("%d", &num2);                         //Enter No 2
        write(sockfd, &num2, sizeof(int));          //Send No 2 to Server

        read(sockfd, &ans, sizeof(int));                //Read Answer from Server
        printf("SERVER: Answer = %d\n", ans);   //Print the Answer
        if(ans == (((int)1) << (8*sizeof(int)-1))-1) printf("WARNING: possibly an overflow\n");
    }
    
    printf("\nYou chose to exit, Thank You so much :)\n");
    close(sockfd);
    return 0;
}
