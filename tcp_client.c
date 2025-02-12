/* TCP/UDP CLIENT FOR TESTING */


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
#include <ctype.h>


// Ensure correct numbers of arguments are provided
int main(int argc, char *argv[]){
    if (argc < 4) {
        fprintf(stderr, "Usage: filename hostname service protocol(TCP/UDP)");
        return 1; 

    }

// Convert provided protocol to uppercase 
    char protocol [4];
    protocol[3] = '\0'; 
    int i;
    for( i = 0; i < 3; ++i){
        protocol[i] = toupper(argv[3][i]); 
    }

    // Configure socket hints 
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    if (!(strcmp(protocol, "TCP")))
    hints.ai_socktype = SOCK_STREAM;
    else if (!(strcmp(protocol, "UDP")))
    hints.ai_socktype = SOCK_DGRAM;
    else {
        fprintf(stderr, "Usage: filename hostname service protocol(TCP/UDP)");
        return 1;
    }

    // Get addres information for peer 
    struct addrinfo *server_addr;
    if (getaddrinfo(argv[1], argv[2], &hints, &server_addr)){
        fprintf(stderr, "call to getaddrinfo() failed. (%d)\n", errno);
        return 1;
    }

    // Create socket 
    int socket_server;
    socket_server = socket(server_addr->ai_family, server_addr->ai_socktype, server_addr->ai_protocol);
    if (!(socket_server >= 0)){
        fprintf(stderr, "call to socket() failed. (%d)\n", errno);
        return 1;
    }
    

    // Establish connection 
    if (connect(socket_server, server_addr->ai_addr, server_addr->ai_addrlen)){
        fprintf(stderr, "call to connect() failed. (%d)\n", errno);
        return 1;
    }

    
    freeaddrinfo(server_addr);

    printf("Enter the messge u wish to send to server and confirm by pressing enter. \n");


    // Wait for user input or incoming data 
    while(1){
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(socket_server, &reads);
        FD_SET(0, &reads);

        if (select(socket_server+1, &reads, 0, 0, 0) < 0 ){
            fprintf(stderr, "call to select() failed. (%d)", errno);
            return 1;
        }

        // Check for received data  
        if (FD_ISSET(socket_server, &reads)){
            char read[4096];
            int bytes_received = recv(socket_server, read, 4096, 0);
            if (bytes_received < 1){
                printf("Connection closed by server.\n");
                break;
            }
            printf("Received (%d bytes): %.*s", bytes_received, bytes_received, read);

        }
        // Check for user input  
        if (FD_ISSET(0, &reads)) {
            char read[4096];
            if (!fgets(read, 4096, stdin)) break;
            printf("Sending: %s", read);
            int bytes_sent = send(socket_server, read, strlen(read), 0);
            printf("Sent %d bytes. \n", bytes_sent);
        }

    }    

    close(socket_server); 
    return 0;

}
