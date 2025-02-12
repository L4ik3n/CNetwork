#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>


int main(int argv, char *argc[]){

    // Configure socket hints 
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo* server_addr;
    if (getaddrinfo("127.0.0.1", "8080", &hints, &server_addr)){
        fprintf(stderr, "call to getaddrinfo() failed. (%d)\n", errno);
        return 1; 
    }

    printf("Remote address is: ");
    char addr_buffer[100];
    char srvc_buffer[100];
    getnameinfo(server_addr->ai_addr, server_addr->ai_addrlen, addr_buffer, sizeof(addr_buffer), srvc_buffer, sizeof(srvc_buffer), NI_NUMERICHOST | NI_NUMERICSERV);
    printf("%s %s\n", addr_buffer, srvc_buffer);


    // Create socket 
    int socket_server;
    socket_server = socket(server_addr->ai_family, server_addr->ai_socktype, server_addr->ai_protocol);
    if (socket_server < 0){
        fprintf(stderr, "call to socket() failed. (%d)\n", errno);
        return 1;
    }

    // Send data 
    const char* mssg = "Hello world\n";
    int bytes_sent = sendto(socket_server, mssg, strlen(mssg), 0, server_addr->ai_addr, server_addr->ai_addrlen);

    // Clean 
    freeaddrinfo(server_addr);
    close(socket_server);
    return 0;  

}