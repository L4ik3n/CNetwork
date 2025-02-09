#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>


int main(){

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    
    struct addrinfo *bind_addr;
    getaddrinfo(0,"8080", &hints, &bind_addr);

    int socket_listen;
    socket_listen = socket(bind_addr->ai_family, bind_addr->ai_socktype, bind_addr->ai_protocol);

    if (socket_listen < 0){
        fprintf(stderr, "call to socket() failed. (%d)\n", errno);
        return 1;

    }

    if (bind(socket_listen, bind_addr->ai_addr, bind_addr->ai_addrlen)){
        fprintf(stderr, "call to bind() failed. (%d)\n", errno);
        return 1; 
    }

    freeaddrinfo(bind_addr);

    fd_set mfds; 
    FD_ZERO(&mfds);
    FD_SET(socket_listen, &mfds);
    int max_socket = socket_listen; 


    while(1){
        fd_set rfds;
        rfds= mfds; 
        if (select(max_socket + 1, &rfds, 0, 0, 0) < 0){
            fprintf(stderr, "call to select() failed. (%d)\n", errno);
            return 1;
        }
        
        if (FD_ISSET(socket_listen, &rfds)){
            struct sockaddr_storage client_addr;
            socklen_t client_len = sizeof(client_addr);
            char read[1024];
            int bytes_received = recvfrom(socket_listen, read, 1024, 0, (struct sockaddr*) &client_addr, &client_len);
            if (bytes_received < 1) {
                fprintf(stderr, "connection closed. (%d)\n", errno);
                return 1;
            }
            printf("Received %d bytes: %.*s", bytes_received, bytes_received, read);

            sendto(socket_listen, read, bytes_received, 0, (struct sockaddr*) &client_addr, client_len); 

        }
    }
    close(socket_listen);
    return 0;


}