/* TIME SERVER (UNIX-BASED)*/


#define _GNU_SOURCE
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
 


int main() {

    // configure local address for web server to bind to 
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address; 
    getaddrinfo(0, "8080", &hints, &bind_address);

    // create socket 
    int socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if (!(socket_listen >=0)){
        fprintf(stderr, "call to socket() failed. (%d)\n", errno);
        return 1;
    }

    // bind socket to local address (bind returns 0 on success, non-zero on failure)
    if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)){
        fprintf(stderr, "call to bind() failed. (%d)\n", errno);
        return 1;
    }
    freeaddrinfo(bind_address);

    // start listening 
    if (listen(socket_listen, 10) < 0){
        fprintf(stderr, "call to listen() failed. (%d)\n", errno);
        return 1;
    }

    // accept incoming connections 
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    int socket_client = accept(socket_listen, (struct sockaddr*) &client_address, &client_len);
    if (!(socket_client >= 0)){
        fprintf(stderr, "call to accept() failed. (%d)\n", errno);
        return 1;
    }

    // get client address 
    char address_buffer[100];
    getnameinfo((struct sockaddr*)&client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    


    // read request 
    char request[1024];
    int bytes_received = recv(socket_client, request, 1024, 0);

    // send response
    const char *response = 
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "Local time is: ";
    int bytes_sent = send(socket_client, response, strlen(response), 0);
    printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));

    time_t seconds;
    time(&seconds);
    char *time_msg = ctime(&seconds);
    bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);
    printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(time_msg));
    
    
    // log to a file 
    FILE *fp;
    if (fp != NULL){
        fp = fopen("time_server.log", "a");
        fprintf(fp, "%s\nConnection from: %s\n", time_msg, address_buffer);
        fprintf(fp, "Received: %d bytes.\n", bytes_received);
        fprintf(fp, "%.*s", bytes_received, request);
        fclose(fp);
    }
    
    close(socket_client);

    return 0; 



}