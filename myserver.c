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
#define MAX_USERNAME_LEN 10
#define MIN_USERNAME_LEN 4

/*
TODO:
1. ability to receive input and allow host to input at same time -- DONE
    - will utilize threads: 1 for each functionality
2. functionality to multicast to each user connected to host/server -- DONE
3. Get usernames from client via a prompt and name clients to differentiate one from another -- DONE
4. make it compatible with Windows and Unix like systems
5. Need to free the client linked list when done using it -- DONE
*/


void* getIncommingInput(void*);
void beginHostInput();
void* getHostInput();
void* writeToAllClients(void*);
pthread_once_t once = PTHREAD_ONCE_INIT;

typedef struct Client {
    int connfd;
    char username[MAX_USERNAME_LEN];
    struct Client* next;
} Client;

void getClientUsername(Client*);

typedef struct Message {
    char* msg;
    Client client;
} Message;

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
            new_client->next = head_client;
            head_client = new_client;
        }
        // fills in the username member of the new client

        printf("Successfully accepted new client! Total clients: %d\n", client_count);

        getClientUsername(head_client);
        printf("New client: %s\n", head_client->username);


        pthread_t incommingClientThreads;
        pthread_once(&once, beginHostInput);
        pthread_create(&incommingClientThreads, NULL, getIncommingInput, head_client);
    }

    close(sockfd);
}
void beginHostInput() {
    pthread_t hostInputThread;
    pthread_create(&hostInputThread, NULL, getHostInput, NULL);
}

void* getHostInput() {
    pthread_detach(pthread_self());
    char msg[MAX_MESSAGE_LEN];
    while(client_count >= 1) {
        pthread_t writeToAllClientsThread;
        fgets(msg, MAX_MESSAGE_LEN, stdin); // the blocking function for this thread
        Client client = { .connfd = -1, .username = "Host" };
        Message host_msg = { .client = client, .msg = msg };
        pthread_create(&writeToAllClientsThread, NULL, writeToAllClients, (void*) &host_msg);
    }
    printf("No more clients...\n");
    pthread_exit(NULL);
}

void* getIncommingInput(void* client_arg) {
    pthread_detach(pthread_self());
    Client client = *((Client*) client_arg);
    char buff[MAX_MESSAGE_LEN];
    int numbytes;
    while(1) {
        pthread_t writeToClientsThread;
        if ((numbytes = recv(client.connfd, &buff, MAX_MESSAGE_LEN - 1, 0)) == -1) {
            perror("server: recv\n");
            pthread_mutex_lock(&client_count_mutex);
            client_count--;
            pthread_mutex_unlock(&client_count_mutex);
            pthread_exit(NULL);
        } else if (numbytes == 0) {
            printf("Client closed connection...\n");
            pthread_mutex_lock(&client_count_mutex);
            client_count--;
            pthread_mutex_unlock(&client_count_mutex);
            pthread_exit(NULL);
        } else {
            buff[numbytes] = '\0';
            Message client_msg = { .client = client, .msg = buff };
            pthread_create(&writeToClientsThread, NULL, writeToAllClients, (void*) &client_msg);
        }
    }
}

void* writeToAllClients(void* msg_arg) {
    pthread_detach(pthread_self());
    Client* prev_client;
    Client* client;
    Message msg_container = *((Message*) msg_arg);
    char new_msg[MAX_MESSAGE_LEN + MAX_USERNAME_LEN];
    sprintf(new_msg, "%s: %s", msg_container.client.username, msg_container.msg); // combining the username and the message into one string to send
    printf("%s", new_msg); // for the host

    pthread_mutex_lock(&head_client_mutex);
    client = head_client;
    prev_client = head_client;
    pthread_mutex_unlock(&head_client_mutex);

    // looping the the linked list of clients
    for (; client != NULL; client = client->next) {
        int connfd = client->connfd;
        if (msg_container.client.connfd == connfd) 
            continue;
        if ((send(connfd, new_msg, strlen(new_msg), 0)) == -1) {
            perror("server: send -- write to all clients\n");
            pthread_mutex_lock(&head_client_mutex);
            prev_client->next = client->next; // removing the client from the list of clients
            free(client); // free the client
            client = prev_client;
            pthread_mutex_unlock(&head_client_mutex);
            close(connfd);
        } else {
            prev_client = client;
        }
        
    }
    pthread_exit(NULL);
}

void getClientUsername(Client* client) {
    char prompt[100];
    sprintf(prompt, "Please enter a username no longer than %d and no shorter than %d characters: ", MAX_USERNAME_LEN, MIN_USERNAME_LEN);
    int numbytes = 0;
    while (numbytes >= MAX_USERNAME_LEN || numbytes <= 3) {
        if ((send(client->connfd, prompt, strlen(prompt), 0)) == -1) {
            perror("server: send - client username\n");
            return;
            // i will let the writeToAllClients handle the removal of the client from the Linked List of clients
        }
        if ((numbytes = recv(client->connfd, client->username, MAX_USERNAME_LEN - 1, 0)) == -1) {
            perror("server: recv - client username\n");
            return;
        } else if (numbytes == 0) {
            return;
        }
    }
    // this is my confirmation send to tell client the username was valid
    if ((send(client->connfd, "y", 1, 0)) == -1) {
            perror("server: send - client username confirmation\n");
            return;
            // i will let the writeToAllClients handle the removal of the client from the Linked List of clients
        }
    client->username[numbytes] = '\0';
}
