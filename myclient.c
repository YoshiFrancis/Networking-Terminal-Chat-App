#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_MSG_LEN 100

void* getUserInput();
void* getServerInput();
static int sockfd;

/*
TODO:
1. make thread for receiving input from server and ready to display
    - must also be ready to cancel the chance for user to input when recv == 0 indicating a FIN
2. make thread for user inputting own message
*/

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
    int status, numbytes;
    pthread_t cliInputThread, servOutputThread;

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
        int iret1, iret2;
        iret1 = pthread_create(&cliInputThread, NULL, getUserInput, NULL);
        iret2 = pthread_create(&servOutputThread, NULL, getServerInput, NULL);

        pthread_join(cliInputThread, NULL);
        pthread_join(servOutputThread, NULL);

        printf("Threads have been joined!");
        

    }

    close(sockfd);
    

    return 0;

}

void* getServerInput() {
    while(1) {
        char buff[MAX_MSG_LEN];
        int numbytes;
        if ((numbytes = recv(sockfd, buff, MAX_MSG_LEN - 1, 0)) == -1) {
            perror("client: recv\n");
            close(sockfd);
            exit(1);
        } else if (numbytes == 0) {
            close(sockfd);
            exit(1);
        }
        buff[numbytes] = '\0';
        printf("\nServer: %s", buff);
    }
}

void* getUserInput() {
    while (1) {
        char msg[MAX_MSG_LEN];
        printf("=");
        fgets(msg, MAX_MSG_LEN, stdin);

        if ((send(sockfd, msg, strlen(msg), 0)) == -1) {
            perror("client: send\n");
            close(sockfd);
            exit(1);
        }
    }
}