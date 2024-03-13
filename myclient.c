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

void* getUserInput(void*);
void* getServerInput(void*);

/*
TODO:
1. make thread for receiving input from server and ready to display -- DONE
    - must also be ready to cancel the chance for user to input when recv == 0 indicating a FIN
2. make thread for user inputting own message -- DONE
3. Make functionality to send username when prompted (might just be all work on the server side)
4. Make compatible with Windows and Unix like systems
*/

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Incorrect usage of the program: input the target ip address after executable!\n");
        return 0;
    }

    struct addrinfo hints, *res, *p;
    int status, numbytes, sockfd;
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
        pthread_create(&cliInputThread, NULL, getUserInput, &sockfd);
        pthread_create(&servOutputThread, NULL, getServerInput, &sockfd);
    }
    close(sockfd);
    return 0;

}

void* getServerInput(void* sockfd_arg) {
    while(1) {
        pthread_detach(pthread_self());
        int sockfd = *((int*)(sockfd_arg)); // giving thread own copy to file descriptor sockfd
        char buff[MAX_MSG_LEN];
        int numbytes;
        
        if ((numbytes = recv(sockfd, buff, MAX_MSG_LEN - 1, 0)) == -1) {
            perror("client: recv\n");
            close(sockfd);
            exit(1);
        } else if (numbytes == 0) {
            close(sockfd);
            exit(1);
        } else {
            buff[numbytes] = '\0';
            printf("\nServer: %s", buff);
        }
        
    }
}

void* getUserInput(void* sockfd_arg) {
    while (1) {
        pthread_detach(pthread_self());
        int sockfd = *((int*)(sockfd_arg)); // giving thread own copy to file descriptor sockfd
        char msg[MAX_MSG_LEN];
        fgets(msg, MAX_MSG_LEN, stdin);

        if ((send(sockfd, msg, strlen(msg), 0)) == -1) {
            perror("client: send\n");
            close(sockfd);
            exit(1);
        }
    }
}