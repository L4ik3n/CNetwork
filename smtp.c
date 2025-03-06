#define _GNU_SOURCE
#define MAXINPUT 512
#define MAXRESPONSE 1024


#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void get_input(const char *prompt, char *buffer);
void send_format(int server, const char *text, ...);
int parse_response(const char *response);
void wait_on_response(int server, int expecting);
int connect_to_host(const char *host, const char *port);

int main(int argc, char *argv[]){
    
    char hostname[MAXINPUT];
    get_input("mail server: ", hostname);

    printf("Connecting to host: %s:25\n", hostname);

    int server = connect_to_host(hostname, "25");

    wait_on_response(server, 220);
    send_format(server, "HELO ANONYMOUS\r\n");
    wait_on_response(server, 250);

    char sender[MAXINPUT];
    get_input("from: ", sender);
    send_format(server, "MAIL FROM:<%s>\r\n", sender);
    wait_on_response(server, 250);

    char recipient[MAXINPUT];
    get_input("to: ", recipient);
    send_format(server, "RCPT TO:<%s>\r\n",recipient);
    wait_on_response(server,250);

    send_format(server, "DATA\r\n");
    wait_on_response(server, 354);

    char subject[MAXINPUT];
    get_input("subject: ", subject);

    send_format(server, "From:<%s>\r\n", sender);
    send_format(server, "To:<%s>\r\n", recipient);
    send_format(server, "Subject:%s\r\n", subject);

    time_t timer;
    time(&timer);

    struct tm *timeinfo;
    timeinfo = gmtime(&timer);

    char date[128];
    strftime(date, 128, "%a, %d %b %Y %H:%M:%S +0000", timeinfo);
    
    send_format(server, "Date:%s\r\n", date);
    send_format(server, "\r\n");

    printf("Enter your email text, end with \".\" on a line by itself.\n");

    while(1){
        char body[MAXINPUT];
        get_input("> ", body);
        send_format(server, "%s\r\n", body);
        if (strcmp(body, ".") == 0) break; 
    }

    wait_on_response(server, 250);

    send_format(server, "QUIT\r\n");
    wait_on_response(server, 221);

    close(server);
    
    return 0;
}


void get_input(const char *prompt, char *buffer){
    printf("%s", prompt);

    buffer[0] = 0;
    fgets(buffer, MAXINPUT, stdin);
    const int read = strlen(buffer);
    if (read > 0) buffer[read - 1] = 0; 
}

void send_format(int server, const char *text, ...){

    char buffer[1024];
    va_list args;
    va_start(args,text);
    vsprintf(buffer,text,args);
    va_end(args);

    send(server, buffer, strlen(buffer), 0);

    printf("C: %s", buffer);

}

int parse_response(const char *response){
    const char *k = response; 
    if (!k[0] || !k[1] || !k[2]) return 0;
    for(;k[3]; ++k){
        if (k == response || k[-1] == '\n') {
            if (isdigit(k[0]) && isdigit(k[1]) && isdigit(k[2])) {
                if (k[3] != '-'){
                    if (strstr(k, "\r\n")) {
                        return strtol(k, 0, 10);
                    }
                }
            }
        }
    }
    return 0;
}


void wait_on_response(int server, int expecting){
    char response[MAXRESPONSE + 1];
    char *p = response; 
    char *end = response + MAXRESPONSE;
    int code = 0; 

    do {
        int bytes_received = recv(server, p, end - p, 0);
        if (bytes_received < 1){
            fprintf(stderr, "Connection dropped.\n");
            exit(1);
        }

        p += bytes_received;
        *p = 0; 
        if (p == end){
            fprintf(stderr, "Server response too large:\n");
            fprintf(stderr, "%s", response);
            exit(1);
        }

        code = parse_response(response);
    } while (code == 0);

    if (code != expecting){
        fprintf(stderr, "Error from server:\n");
        fprintf(stderr, "%s", response);
        exit(1);
    }

    printf("S: %s", response);

}

int connect_to_host(const char *host, const char *port){
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_addr; 
    if (getaddrinfo(host, port, &hints, &peer_addr)){
        fprintf(stderr, "Call to getaddrinfo() failed. (%d)\n", errno);
        exit(1);
    }

    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_addr->ai_addr, peer_addr->ai_addrlen, address_buffer, sizeof(address_buffer), service_buffer, sizeof(service_buffer), NI_NUMERICHOST);
    printf("%s %s\n", address_buffer, service_buffer);


    int server; 
    server = socket(peer_addr->ai_family, peer_addr->ai_socktype, peer_addr->ai_protocol);
    if (server < 0){
        fprintf(stderr, "Call to socket() failed. (%d)\n", errno);
        exit(1);
    }

    if (connect(server, peer_addr->ai_addr, peer_addr->ai_addrlen)){
        fprintf(stderr, "Call to connect failed. (%d)\n", errno);
        exit(1);
    }
    freeaddrinfo(peer_addr);

    printf("Connected. \n\n");

    return server;
}