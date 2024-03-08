#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_MSG_LEN 100

int main(int argc, char* argv[]) {
    // getaddrinfo
    // socket
    // connect
    // read/write
    // close

    if (argc != 2) {
        printf("Usage of the program is to import the target ip address after executable!\n");
        exit(0);
    }

    struct addrinfo hints, *res, *p;
    int sockfd, status, numbytes;
    char buff[MAX_MSG_LEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ((status = getaddrinfo(argv[1], "4900", &hints, &res)) != 0) {
        perror("client: getaddrinfo\n");
        exit(1);
    }

    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket\n");
            continue;
        }
        if ((connect(sockfd, p->ai_addr, p->ai_addrlen)) == -1) {
            perror("client: connect\n");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        perror("client: failed to connect\n");
        exit(1);
    }
    freeaddrinfo(res);

    while(1) {
        if ((numbytes = recv(sockfd, buff, MAX_MSG_LEN - 1, 0)) == -1) {
            perror("client: recv\n");
            close(sockfd);
            exit(1);
        }
        buff[numbytes] = '\0';
        printf("Server: %s\n", buff);

        char msg[MAX_MSG_LEN];
        printf("Input message: ");
        fgets(msg, MAX_MSG_LEN, stdin);
        printf("sending message...\n");
        if ((send(sockfd, msg, strlen(msg), 0)) == -1) {
            perror("client: send\n");
            close(sockfd);
            exit(1);
        }
    }
    

    
    

    close(sockfd);

    return 0;

}