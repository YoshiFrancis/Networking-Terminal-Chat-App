#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define PORT "4900"
#define BACKLOG 10
#define MAX_MESSAGE_LEN 100

/*
TODO:
1. ability to receive input and allow host to input at same time
    - will utilize threads: 1 for each functionality
2. functionality to multicast to each user connected to host/server
*/


void* getIncommingInput();
void* getHostInput();
static int sockfd, newfd;


int main(int argc, char* argv[]) {

    struct addrinfo hints, *res, *p;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;


    if ((status = getaddrinfo(NULL, PORT, &hints, &res) != 0)) {
        perror("server: get addrinfo\n");
    }

    for (p = res; p != NULL; p = p->ai_next) {

        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("servor: socket\n");
            continue;
        }

        if ((bind(sockfd, p->ai_addr, p->ai_addrlen)) == -1) {
            perror("server: bind\n");
            close(sockfd);
            continue;
        }

        break;
    }

    freeaddrinfo(res);
    
    if (p == NULL) {
        perror("Server: failed to bind\n");
        exit(1);
    }

    
    if ((listen(sockfd, BACKLOG)) == -1) {
        perror("Server: listen\n");
        close(sockfd);
        exit(1);
    }

    printf("Waiting for connections...\n");

    while(1) {
        struct sockaddr_storage client_addr;
        socklen_t client_addr_len = sizeof client_addr;
        if ((newfd = accept(sockfd, (struct sockaddr*) &client_addr, &client_addr_len)) == -1) {
            perror("server: accept\n");
            continue;
        }
        printf("Successfully accepted new client!\n");
        
        // way to differentiate btwn different forks is the pid
        if (!fork()) {
            close(sockfd);
            printf("Beginning communications...\n");
            pthread_t incommingClientThreads, hostOutputThread;
            int iret1, iret2;
            iret1 = pthread_create(&incommingClientThreads, NULL, getIncommingInput, NULL);
            iret2 = pthread_create(&hostOutputThread, NULL, getHostInput, NULL);


            pthread_join(incommingClientThreads, NULL);
            pthread_join(hostOutputThread, NULL);

            printf("Threads have been joined!\n");
            close(newfd);
            exit(0);
        }

        close(newfd);
    }

    close(sockfd);
}

void* getHostInput() {
    while(1) {
        char msg[MAX_MESSAGE_LEN];
        printf("Input messagee: ");
        fgets(msg, MAX_MESSAGE_LEN, stdin);

        if ((send(newfd, msg, strlen(msg), 0)) == -1) {
            perror("server: send\n");
            exit(1);
        }
        printf("Sending message to client...");
    }
}

void* getIncommingInput() {
    while(1) {
        char buff[MAX_MESSAGE_LEN];
        int numbytes;

        if ((numbytes = recv(newfd, &buff, MAX_MESSAGE_LEN - 1, 0)) == -1) {
            perror("server: recv\n");
            exit(1);
        } else if (numbytes == 0) {
            printf("Client closed connection...\n");
            exit(1);
        }

        buff[numbytes] = '\0';
        printf("Client: %s\n", buff);
    }
}
