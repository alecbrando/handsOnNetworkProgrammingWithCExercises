#include "chap03.h"
#include <netdb.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>

typedef struct Client {
    SOCKET s;
    struct sockaddr_storage client_address;
} Client;

typedef struct ClientList {
    Client *clients;
    int capacity;
    int count;
} ClientList;

int addressClient(int client_index, ClientList* client_list);
int main() {
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_flags = AI_PASSIVE;
    hint.ai_socktype = SOCK_STREAM;
    struct addrinfo* server_address;
    if (getaddrinfo(0, "8080",&hint, &server_address) != 0) {
        printf("getaddrinfo failed()");
        return 1;
    }

    SOCKET server_socket = socket(server_address->ai_family, server_address->ai_socktype,server_address->ai_protocol);
    if (server_socket == -1) {
        printf("server_socket failed");
        return 1;
    }
    printf("Socket Created\n");
    if (bind(server_socket, server_address->ai_addr, server_address->ai_addrlen) > 0) {
        printf("Failed to bind socket to ");
        return 1;
    }

    printf("Bind complete\n");
    if (listen(server_socket, 10) < 0) {
        printf("Failed to listen");
        return 1;
    }
    printf("Listen complete\n");

    freeaddrinfo(server_address);
    int max = server_socket;
    struct sockaddr_storage client_address;
    ClientList client_list;
    client_list.capacity = 10;
    client_list.count = 0;
    client_list.clients = malloc(sizeof(Client) * client_list.capacity);
    while(1) {
        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(0, &rd);
        FD_SET(server_socket, &rd);
        for(int i = 0; i < client_list.count; i++) {
            if(client_list.clients[i].s > max) {
                max = client_list.clients[i].s;
            }
            FD_SET(client_list.clients[i].s, &rd);
        }
        select(max + 1, &rd, NULL, NULL, NULL);
        if (FD_ISSET(0, &rd)){
            char ch;
            ch = getchar();
            if(ch == 'q') {
                printf("We are breaking broda");
                break;
            }
            continue;
        }
        if (FD_ISSET(server_socket, &rd)){
            printf("Accepting new client\n");
            socklen_t client_len = sizeof(client_address);
            SOCKET socket_client = accept(server_socket, (struct sockaddr*)&client_address, &client_len);

            if(client_list.count >= client_list.capacity) {
                client_list.capacity *= 2;
                Client *temp_list = realloc(client_list.clients, sizeof(Client) * client_list.capacity);
                if(temp_list == NULL) {
                    printf("Problem reallocing");
                    return 1;
                }
                client_list.clients = temp_list;
            }
            client_list.clients[client_list.count].s = socket_client;
            client_list.clients[client_list.count].client_address = client_address;
            client_list.count++;
        }

        for(int i = 0; i < client_list.count; i++) {
            if(FD_ISSET(client_list.clients[i].s, &rd)) {
                printf("Message from client received\n");
                int res = addressClient(i, &client_list);
                if(res == 1) {
                    // Swap with last client and decrement count
                    client_list.clients[i] = client_list.clients[client_list.count - 1];
                    client_list.clients[client_list.count - 1].s = -1;
                    client_list.count--;
                    i--;
                }
            }
        }
    }
    free(client_list.clients);
    close(server_socket);
    return 0;
}

int addressClient(int client_index, ClientList* client_list) {
    Client* sender = &client_list->clients[client_index];
    socklen_t client_len = sizeof(sender->client_address);

    char address_buffer[100];
    getnameinfo((struct sockaddr*)&sender->client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    printf("Message from client %s\n", address_buffer);

    char request[1024];
    int bytes_received = recv(sender->s, request, 1023, 0);
    if(bytes_received <= 0){
        close(sender->s);
        return 1;
    }
    request[bytes_received] = '\0';

    printf("Preparing to send messages\n");
    for (int j = 0; j < client_list->count; j++) {
        if (j == client_index) {
            printf("Skipping sender\n");
            continue;
        }
        printf("Sending to client %d\n", j);
        send(client_list->clients[j].s, request, bytes_received, 0);
    }

    if (strcasestr(request, "QUIT") != NULL) {
        printf("We are closing connection\n");
        close(sender->s);
        return 1;
    }

    return 0;
}
