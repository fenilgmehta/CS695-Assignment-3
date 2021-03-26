/*
	Server Side
	Please pass port no as command line argument
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
    if (argc != 2) {
        fprintf(stderr, "ERROR: invalid syntax\n");
        printf("Usage: %s <PORT>\n", argv[0]);
        exit(1);
    }

    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[1024];
    struct sockaddr_in serv_addr, cli_addr;

    // socket()
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    // bind()
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    // listen()
    listen(sockfd, 5);

    // accept()
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) error("ERROR on accept");
    else printf("Client successfully connected :)\n\n");

    // START main work
    int num1, num2, ans, choice;
    int n;
    while(1){
        char strMenu[] = "##### Menu #####\n1.Addition\n2.Subtraction\n3.Multiplication\n4.Division\n5.Exit\nEnter your choice: ";
        n = write(newsockfd, strMenu, strlen(strMenu));         //Ask for choice
        if (n < 0) error("ERROR writing to socket");
        read(newsockfd, &choice, sizeof(int));                  //Read choice
        printf("CLIENT: Choice = %d\n", choice);

        if(choice==5){
            break;
        }else if (choice>5){
            char strWrongChoice[] = "Enter a valid choice.";
            n = write(newsockfd, strWrongChoice, strlen(strWrongChoice));
            if (n < 0) error("ERROR writing to socket");
        }

        char strNum1[] = "Enter Number 1: ";
        n = write(newsockfd, strNum1, strlen(strNum1));         //Ask for Number 1
        if (n < 0) error("ERROR writing to socket");
        read(newsockfd, &num1, sizeof(int));                    //Read No 1
        printf("CLIENT: Number 1 = %d\n", num1);

        char strNum2[] = "Enter Number 2: ";
        n = write(newsockfd, strNum2, strlen(strNum2));         //Ask for Number 2
        if (n < 0) error("ERROR writing to socket");
        read(newsockfd, &num2, sizeof(int));                    //Read Number 2
        printf("CLIENT: Number 2 = %d\n", num2);

        switch (choice) {
            case 1:
                ans = num1 + num2;
                break;
            case 2:
                ans = num1 - num2;
                break;
            case 3:
                ans = num1 * num2;
                break;
            case 4:
                if (num2 == 0) {ans = 1; ans = (ans << (8*sizeof(int)-1))-1;}
                else ans = num1 / num2;
                break;
        }

        printf("SERVER: Reply sent to the client = %d\n", ans);
        write(newsockfd, &ans, sizeof(int));
        printf("\n");
    }

    close(newsockfd);
    close(sockfd);
    return 0;
}
