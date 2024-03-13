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


void* getIncommingInput(void*);
void beginHostInput();
void* getHostInput();
pthread_once_t once = PTHREAD_ONCE_INIT;

typedef struct Client {
    int connfd;
    struct Client* next;
} Client;

static Client* head_client;
static int client_count;

pthread_mutex_t head_client_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]) {

    struct addrinfo hints, *res, *p;
    int status, sockfd, newfd;

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
        int* newfd_ptr;
        if ((newfd = accept(sockfd, (struct sockaddr*) &client_addr, &client_addr_len)) == -1) {
            perror("server: accept\n");
            continue;
        }
        if ((newfd_ptr = malloc(sizeof(newfd))) == NULL) {
            perror("server: malloc\n");
            close(newfd);
            continue;
        } 
        *newfd_ptr = newfd;
        client_count++;
        if (head_client == NULL) {
            head_client = malloc(sizeof(Client));
            head_client->connfd = newfd;
            head_client->next = NULL;
        } else {
            Client* new_client = malloc(sizeof(Client));
            new_client->connfd = newfd;
            new_client->next = NULL;
            head_client->next = new_client;
        }
        printf("Successfully accepted new client! Total clients: %d\n", client_count);
        printf("Beginning communications...\n");

        pthread_t incommingClientThreads;
        pthread_once(&once, beginHostInput);
        pthread_create(&incommingClientThreads, NULL, getIncommingInput, newfd_ptr);
        printf("Onto the next...\n");
    }

    close(sockfd);
}
void beginHostInput() {
    pthread_t hostInputThread;
    pthread_create(&hostInputThread, NULL, getHostInput, NULL);
}

void* getHostInput() {
    while(client_count >= 1) {
        Client* prev_client;
        Client* client;
        char msg[MAX_MESSAGE_LEN];
        printf("Input messagee: ");
        fgets(msg, MAX_MESSAGE_LEN, stdin); // the blocking function for this thread
        pthread_mutex_lock(&head_client_mutex);
        client = head_client;
        prev_client = head_client;
        pthread_mutex_unlock(&head_client_mutex);
        for (; client != NULL; client = client->next) {
            int connfd = client->connfd;
            if ((send(connfd, msg, strlen(msg), 0)) == -1) {
                perror("server: send\n");
                pthread_mutex_lock(&head_client_mutex);
                prev_client = client->next; // removing the client from the list of clients
                pthread_mutex_unlock(&head_client_mutex);
                close(connfd);
            }
            prev_client = client;
        }
    }
}

void* getIncommingInput(void* newfd_arg) {
    while(1) {
        int newfd = *((int*) newfd_arg);
        char buff[MAX_MESSAGE_LEN];
        int numbytes;

        if ((numbytes = recv(newfd, &buff, MAX_MESSAGE_LEN - 1, 0)) == -1) {
            perror("server: recv\n");
            pthread_mutex_lock(&client_count_mutex);
            client_count--;
            pthread_mutex_unlock(&client_count_mutex);
            exit(1);
        } else if (numbytes == 0) {
            printf("Client closed connection...\n");
            pthread_mutex_lock(&client_count_mutex);
            client_count--;
            pthread_mutex_unlock(&client_count_mutex);
            exit(1);
        } else {
            buff[numbytes] = '\0';
            pthread_t tid = pthread_self();
            printf("\nClient %lu: %s", tid, buff);
        }
    }
}
