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


int main() {

    // Configure socket hints and get address
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_addr;
    getaddrinfo(0, "8080", &hints, &bind_addr);


    // Create socket
    int socket_listen;
    socket_listen = socket(bind_addr->ai_family, bind_addr->ai_socktype, bind_addr->ai_protocol);

    if (socket_listen < 0 ){
        fprintf(stderr, "call to socket() failed. (%d) ", errno);
        return 1;
    }

    // Bind socket to address  
    if (bind(socket_listen, bind_addr->ai_addr, bind_addr->ai_addrlen)){
        fprintf(stderr, "call to bind() failed. (%d)", errno);
        return 1; 
    }

    freeaddrinfo(bind_addr);

    // Listen for connections 
    if (listen(socket_listen, 10) < 0){
        fprintf(stderr, "call to listen() failed. (%d)\n", errno);
        return 1;
    }

    fd_set mfds;
    FD_ZERO(&mfds);
    FD_SET(socket_listen, &mfds);
    int max_socket = socket_listen;
    
    // Wait for incoming data and echo it to other clients excluding sender 
    while(1){
        fd_set rfds;
        rfds = mfds; 


        if (select(max_socket + 1, &rfds, 0, 0, 0) < 0){
            fprintf(stderr, "call to select() failed. (%d)\n", errno);
            return 1;
        }
        int i; 

        // Loop thru all sockets 
        for(i = 1; i<=max_socket; ++i){
            // Check for new connections 
            if (FD_ISSET(i, &rfds)) {
                if (i == socket_listen) {
                    struct sockaddr_storage client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int socket_client = accept(socket_listen, (struct sockaddr*) &client_addr, &client_len);
                    if (socket_client < 0){
                        fprintf(stderr, "call to accept() failed. (%d)\n", errno);
                        return 1;
                    }
                    FD_SET(socket_client, &mfds);
                    if (socket_client > max_socket) max_socket = socket_client;
                

                } 
            // Read received data and echo it to other clients 
            else{
                    char read[1024];
                    int bytes_received = recv(i, read, 1024, 0);
                    if (bytes_received < 1){
                        FD_CLR(i, &mfds);
                        close(i);
                        continue;
                    }
                int j;
                for(j = 1; j <= max_socket; ++j){
                    if (FD_ISSET(j, &mfds)){
                        if (j == socket_listen || j == i)
                            continue;
                        else 
                            send(j, read, bytes_received, 0);
                    }
                }
            }



            }
        }
    }
    close(socket_listen);
    return 0;


    

}