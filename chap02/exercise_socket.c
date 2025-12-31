#include <netdb.h>          /* getaddrinfo, getnameinfo for address resolution */
#include <string.h>         /* memset, strlen */
#include <stdio.h>          /* printf, fprintf */
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>

typedef struct connect_sockets {
    int socket;
    struct sockaddr_storage client_address;
} connected_sockets;  

int addressClient(int* socket_client, struct sockaddr_storage* client_address, socklen_t client_len, connected_sockets clients[]);

int main() {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    if(getaddrinfo(0, "8080", &hints, &bind_address) != 0) {
        perror("getaddrinfo");
    }

    int listen_socket = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if(listen_socket == -1) {
        perror("socket"); 
        freeaddrinfo(bind_address);
        return 1;
    } 
    
    if(bind(listen_socket, bind_address->ai_addr, bind_address->ai_addrlen)){
        fprintf(stderr, "bind() failed. (%d)\n", errno);
        freeaddrinfo(bind_address);
        return 1;
    }

    freeaddrinfo(bind_address);
    
    if (listen(listen_socket, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", errno);
        freeaddrinfo(bind_address);
        return 1;
    } else {
        printf("Server listening on port 8080...\n");
    }
    
    struct sockaddr_storage client_address;
    connected_sockets clients[8];
    for(int i = 0; i < 8; i++){
        clients[i].socket = -1;
    }
    int max = 0;
    while(1) {
        fd_set rd;
        int socket_client;
        FD_ZERO(&rd);
        FD_SET(listen_socket, &rd);
        max = listen_socket;
        for(int i = 0; i < 8; i++) {
            int temp = clients[i].socket;
            if(temp > max && temp != -1) {
                max = temp;
            }
            if(clients[i].socket != -1){
                FD_SET(clients[i].socket, &rd);
            }
        }
        select(max + 1, &rd, NULL, NULL, NULL);
        if (FD_ISSET(listen_socket, &rd)) {
            socklen_t client_len = sizeof(client_address);
            for (int i = 0; i < 8; i++) {
                if(clients[i].socket == -1) {
                    socket_client = accept(listen_socket, (struct sockaddr*)&clients[i].client_address, &client_len);
                    clients[i].socket = socket_client;
                    break;
                }
            }
        }
        for(int i = 0; i < 8; i++) {
            if (clients[i].socket != -1 && FD_ISSET(clients[i].socket, &rd)) {
                socklen_t client_len = sizeof(client_address);
                int response = addressClient(&clients[i].socket, &clients[i].client_address, client_len, clients);
                if(response == 1){
                    clients[i].socket = -1;
                }
            }
        }
    }

    close(listen_socket);

    return 0;
}


int addressClient(int* socket_client, struct sockaddr_storage* client_address, socklen_t client_len, connected_sockets clients[]){
    char address_buffer[100];
    getnameinfo((struct sockaddr*)client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    printf("New client connected (Client #%s)\n", address_buffer);
    char request[1024];
    int bytes_received = recv(*socket_client, request, 1023, 0);
    if(bytes_received <= 0){
        close(*socket_client);
        return 1;
    }
    request[bytes_received] = '\0';
    int bytes_sent = send(*socket_client, request, strlen(request), 0);
    if (strcasestr(request, "QUIT") != NULL) {
        printf("We are closing connection\n");
        close(*socket_client);
        return 1;
    }
    if (strcasestr(request, "LIST") != NULL) {
        char response[1024] = "Connected clients:\n";
        int bytes_sent = send(*socket_client, response, strlen(response), 0);
        int count = 0;
        for(int i = 0; i < 8; i++){
           if (clients[i].socket != -1) {
                char response[1024];
                sprintf(response, "Client %d\n", ++count);
               int bytes_sent = send(*socket_client, response, strlen(response), 0);
           }
        }
    }

    return 0;
}
