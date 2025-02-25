#define _GNU_SOURCE
#define RESPONSE_SIZE 8192
#define TIMEOUT 5.0


#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>




void parse_url(char *url, char **hostname, char **port, char **path);
void send_request(int sock, char *hostname, char *port, char *path);
int connect_to_server(char *hostname, char *port);

int main(int argc, char *argv[]){

    if (argc < 2) {
        fprintf(stderr, "Usage: %s URL", argv[0]);
        return 1;
    }
        
    char *url = argv[1];

    char *hostname, *port, *path;
    parse_url(url, &hostname, &port, &path);

    int server = connect_to_server(hostname, port);
    send_request(server, hostname, port, path);

    const clock_t start_time = clock();
    
    char response[RESPONSE_SIZE + 1];
    char *p = response, *q;
    char *end = response + RESPONSE_SIZE;
    char *body = 0;
    
    enum {length, chunked, connection};
    int encoding = 0;
    int remaining = 0;

    while(1){

        if ((clock() - start_time) / CLOCKS_PER_SEC > TIMEOUT){
            fprintf(stderr, "timeout after %.2f seconds\n", TIMEOUT);
            return 1;
        }

        if (p == end){
            fprintf(stderr, "out of buffer space\n");
            return 1;
        } 

        fd_set reads; 
        FD_ZERO(&reads);
        FD_SET(server, &reads);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 200000;

        if (select(server + 1, &reads, 0, 0, &timeout) < 0){
            fprintf(stderr, "call to select failed. (%d)\n", errno);
            return 1;
        }

        if (FD_ISSET(server, &reads)){
            int bytes_received = recv(server, p, end - p, 0);
            if (bytes_received < 1){
                if (encoding == connection && body){
                    printf("%.*s", (int)(end-body), body);
                }

                printf("Connection closed by host");
                break; 
            }

            p += bytes_received;
            *p = 0; 

            if (!body && (body = strstr(response, "\r\n\r\n"))){
                *body = 0;
                body += 4;
                
                printf("Received Headers:\n%s\n", response);

                q = strstr(response, "\nContent-Length: ");
                if (q){
                    encoding = length;
                    q = strchr(q, ' ');
                    q += 1;
                    remaining = strtol(q, 0, 10);
                } else{
                    q = strstr(response, "\nTransfer-Encoding: chunked");
                    if (q){
                        encoding = chunked; 
                        remaining = 0;
                    } else{
                        encoding = connection;
                    }
                }
                
                printf("\nReceived Body:\n");
            }

                if (body){
                    if (encoding == length) {
                        if (p - body >= remaining) {
                            printf("%.*s", remaining, body);
                            break;
                        } 

                    } else if ( encoding == chunked){
                        do {
                            if (remaining == 0){
                                if ((q = strstr(body, "\r\n"))){
                                    remaining = strtol(body, 0, 16);
                                    if (!remaining) goto finish;
                                    body = q + 2;
                                } else{
                                    break;
                                }
                            }
                            if (remaining && p - body >= remaining){
                                printf("%.*s", remaining, body);
                                body += remaining + 2;
                                remaining = 0;
                            }

                        } while(!remaining);
                    }
                }
            
        }

    }

    finish: 
        close(server);
        printf("Finished.\n");
    
        return 0;
}



void parse_url(char *url, char **hostname, char **port, char **path){
    char *p; 
    p = strstr(url, "://");

    char *protocol = 0; 
    if (p){
        protocol = url;
        *p = 0;
        p += 3;
    } else{
        p = url; 
    }

    if (protocol){
        if(strcmp(protocol, "http")){
            fprintf(stderr, "Only http is supported.\n");
            exit(1);
        }
    }

    *hostname = p;
    while (*p && *p != ':' && *p != '/' && *p != '#') ++p;


    *port = "80";
    if (*p == ':'){
        *p++ = 0;
        *port = p;
    }
    while (*p && *p != '/' && *p != '#') ++p;

    *path = p;
    if (*p == '/'){
        *path = p + 1;
    }

    *p = 0;

    while (*p && *p != '#') ++p;
    if (*p == '#') *p = 0;

    printf("Hostname: %s\n", *hostname);
    printf("Port: %s\n", *port);
    printf("Path: %s\n", *path);


}


void send_request(int sock, char *hostname, char *port, char *path){
    
    char buffer[2048];

    sprintf(buffer, "GET /%s HTTP/1.1\r\n", path);
    sprintf(buffer + strlen(buffer), "Host: %s:%s\r\n", hostname, port);
    sprintf(buffer + strlen(buffer), "Connection: close\r\n");
    sprintf(buffer + strlen(buffer), "User-Agent: me\r\n");
    sprintf(buffer + strlen(buffer), "\r\n");

    send(sock, buffer, strlen(buffer), 0);
    printf("Sent Headers:\n%s", buffer);


}


int connect_to_server(char *hostname, char *port){

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *server_addr; 
    if (getaddrinfo(hostname, port, &hints, &server_addr)){
        fprintf(stderr, "call to getaddrinfo() failed. (%d)\n", errno);
        exit(1);
    }

    printf("Remote address is: ");
    char addr_buffer[1024];
    char srvc_buffer[1024];
    getnameinfo(server_addr->ai_addr, server_addr->ai_addrlen, addr_buffer, sizeof(addr_buffer), srvc_buffer, sizeof(srvc_buffer), NI_NUMERICHOST);
    printf("%s %s\n", addr_buffer, srvc_buffer);


    int server; 
    server = socket(server_addr->ai_family, server_addr->ai_socktype, server_addr->ai_protocol);
    if (server < 0) {
        fprintf(stderr, "call to socket() failed. (%d)\n", errno);
        exit(1);
    }

    if (connect(server, server_addr->ai_addr, server_addr->ai_addrlen)){
        fprintf(stderr, "call to connect() failed. (%d)\n", errno);
        exit(1);
    }

    freeaddrinfo(server_addr);

    printf("Connected.\n\n");
    return server;


}
