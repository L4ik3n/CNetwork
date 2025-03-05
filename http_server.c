#define _GNU_SOURCE
#define MAX_REQUEST_SIZE 2047
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>



int create_socket(const char *host, const char *port);
fd_set wait_on_clients(int server);
client_info *get_client(int socket);
const char *get_client_address(client_info *ci);
const char *get_content_type(const char *path);
void drop_client(client_info *client);
void send_400(client_info *client);
void send_404(client_info *client);
void serve_resource(client_info *client, const char *path);

typedef struct {
    socklen_t address_length;
    struct sockaddr_storage address;
    int socket;
    char request[MAX_REQUEST_SIZE + 1];
    int received;
    struct client_info *next;
} client_info;

static struct client_info *clients = 0; 



int main(int argc, char* argv[]){
    int server = create_socket("127.0.0.1", "8080");

    while(1){

        fd_set rfds;
        rfds = wait_on_clients(server);    
        
        if (FD_ISSET(server, &rfds)){
            client_info *client = get_client(-1);

            client->socket = accept(server, (struct sockaddr*) &(client->address), &(client->address_length));
            if (client->socket < 0){
                fprintf(stderr, "Call to accept failed. (%d)\n", errno);
                return 1;
            }

            printf("New connection from: %s.\n", get_client_address(client));
        }
        client_info *client = clients;
        while(client){
            client_info *next = client->next;

            if (FD_ISSET(client->socket, &rfds)){
                if (MAX_REQUEST_SIZE == client->received){
                    send_400(client);
                    continue;
                }

                int r = recv(client->socket, client->request + client->received, MAX_REQUEST_SIZE - client->received, 0);
                if (r < 1){
                    printf("Unexpected disconnect from %s.\n", get_client_address(client));
                    drop_client(client);
                } else {
                    client->received +=r;
                    client->request[client->received] = 0;

                    char *q = strstr(client->request, "\r\n\r\n");

                    if (q){
                        if (strncmp("GET /", client->request, 5)){
                            send_400(client);
                        } else{
                            char *path = client->request +4;
                            char *end_path = strstr(path, " ");
                            if (!end_path){
                                send_400(client);
                            } else {
                                *end_path = 0;
                                serve_resource(client, path);
                            }
                        }
                    }
                }
            }
            
            client = next;        
        }
    }
    // add code to shut down server 
    close(server);

    return 0;
}



int create_socket(const char *host, const char *port){
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(host, port, &hints, &bind_address);

    int socket_server;
    socket_server = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if (socket_server < 0) {
        fprintf(stderr, "Call to socket() failed. (%d)\n", errno);
        exit(1);
    }

    freeadrrinfo(bind_address);

    if (listen(socket_server, 10) < 0){
        fprintf(stderr, "Call to listen() failed. (%d)\n");
        exit(1);
    }

    return socket_server;

}


fd_set wait_on_clients(int server){

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(server, &rfds);
    int max_socket = server;

    client_info *ci = clients;

    while(ci){
        FD_SET(ci->socket, &rfds);
        if (ci->socket > max_socket) max_socket = ci->socket;
        ci = ci->next; 
    }

    if (select(max_socket+1, &rfds, 0, 0, 0) < 0){
        fprintf(stderr, "Call to select() failed. (%d)\n", errno);
        exit(1);
    }
    return rfds;
}

client_info *get_client(int s){
    
    client_info *ci = clients;

    while(ci){
        if (ci->socket == s) break; 
        ci = ci->next;
    }
    if (ci) return ci;

    client_info *n = (client_info*) calloc(1, sizeof(client_info));

    if (!n){
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }

    n->address_length = sizeof(n->address);
    n->next = clients;
    clients = n;
    return n;
}


const char *get_client_address(client_info *ci){
    static char address_buffer[100];
    getnameinfo((struct sockaddr*)&ci->address, ci->address_length, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    return address_buffer;
}


const char *get_content_type(const char *path){
    const char *last_dot = strrch(path, '.');
    if (last_dot){
        if (!(strcmp(last_dot, ".css"))) return "text/css";
        if (!(strcmp(last_dot, ".csv"))) return "text/csv";
        if (!(strcmp(last_dot, ".gif"))) return "image/gif";
        if (!(strcmp(last_dot, ".htm"))) return "text/html";
        if (!(strcmp(last_dot, ".html"))) return "text/html";
        if (!(strcmp(last_dot, ".ico"))) return "image/x-icon";
        if (!(strcmp(last_dot, ".jpeg"))) return "image/jpeg";
        if (!(strcmp(last_dot, ".jpg"))) return "image/jpeg";
        if (!(strcmp(last_dot, ".js"))) return "application/javascript";
        if (!(strcmp(last_dot, ".json"))) return "application/json";
        if (!(strcmp(last_dot, ".png"))) return "image/png";
        if (!(strcmp(last_dot, ".pdf"))) return "application/pdf";
        if (!(strcmp(last_dot, ".svg"))) return "image/svg+xml";
        if (!(strcmp(last_dot, ".txt"))) return "text/plain";
    }

    return "application/octet-stream";
}


void drop_client(client_info *client){

    close(client->socket);
    client_info **p = &clients;

    while(*p){
        if (*p == client) {
            *p = client->next;
            free(client);
            return;
        }
        p = &(*p)->next;
    }

    fprintf(stderr, "Client not found.\n");
    exit(1);
}

void send_400(client_info *client){
    const char *c400 = "HTTP/1.1 400 Bad Request\r\n"
        "Connection: close\r\n"
        "Content-Length: 11\r\n\r\nBad Request";
    send(client->socket, c400, sizeof(c400), 0);
    drop_client(client);
}

void send_404(client_info *client){
    const char *c404 = "HTTP/1.1 404 Not Found\r\n"
        "Connection: close\r\n"
        "Content-Length: 9\r\n\r\nNot Found";
        send(client->socket, c404, strlen(c404), 0);
        drop_client(client);

}

void serve_resource(client_info *client, const char *path){

    // add user agent, date, time, response code
    printf("Serve resource %s %s\n", get_client_address(client), path);

    if (!(strcmp(path, "/"))) path = "/index.html";
    if (strlen(path) > 100){
        send_400(client);
        return;
    }

    if (strstr(path, "..")) {
        send_404(client);
        return;
    }

    char full_path[128];
    sprintf(full_path, "public%s", path);

    FILE *fp = fopen(full_path, "rb");

    if (!fp) {
        send_404(client);
        return;
    }

    fseek(fp, 0L, SEEK_END);
    size_t cl = ftell(fp);
    rewind(fp);
    const char *ct = get_content_type(full_path);

#define BSIZE 1024

    char buffer[BSIZE];

    sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    send(client->socket, buffer, strlen(buffer), 0);
    
    sprintf(buffer, "Connection: close\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Content-Length: %u\r\n", cl);
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Content-Type: %s\r\n", ct);
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    int r = fread(buffer, 1, BSIZE, fp);
    while(r){
        send(client->socket, buffer, r, 0);
        r = fread(buffer, 1, BSIZE, fp);
    }

    fclose(fp);
    drop_client(client);

}
