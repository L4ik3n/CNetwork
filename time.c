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
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
 


int main() {


    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx){
        fprintf(stderr, "Call to SSL_CTX_new() failed.\n");
        return 1;
    }

    if (!SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) || !SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM)){
        fprintf(stderr, "Call to SSL_CTX_use_certificate_file() failed.\n");
        ERR_print_errors_fp(stderr);
        return 1;
    } 





    // configure local address for web server to bind to 
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
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

    // clear IPV6_V6ONLY flag
    int option = 0; 
    if (setsockopt (socket_listen, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&option, sizeof(option))){
        fprintf(stderr, "call to setsockopt() failed. (%d)\n", errno);
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

     
    while(1){
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


        SSL *ssl = SSL_new(ctx);
        if (!ctx) {
            fprintf(stderr, "call to SSL_new() failed.\n");
            return 1;
        }

        SSL_set_fd(ssl, socket_client);
        if (SSL_accept(ssl) <= 0){
            fprintf(stderr, "Call to SSL_accept() failed.\n");
            ERR_print_errors_fp(stderr);

            SSL_shutdown(ssl);
            close(socket_client);
            SSL_free(ssl);
            continue;
        }

        printf("SSL connection using %s\n", SSL_get_cipher(ssl));


        // read request 
        char request[1024];
        int bytes_received = SSL_read(ssl, request, 1024);

        // send response
        const char *response = 
            "HTTP/1.1 200 OK\r\n"
            "Connection: close\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Local time is: ";
        int bytes_sent = SSL_write(ssl, response, strlen(response));
        printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));


        
        time_t seconds;
        time(&seconds);
        char *time_msg = ctime(&seconds);
        bytes_sent = SSL_write(ssl, time_msg, strlen(time_msg));
        printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(time_msg));

        char *ip_mssg = "\nYour IP address is: ";
        bytes_sent = SSL_write(ssl, ip_mssg, strlen(ip_mssg));
        printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(time_msg));
        bytes_sent = SSL_write(ssl, address_buffer, strlen(address_buffer));
        printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(time_msg));
          
        // log to a file 
        FILE *fp;
        fp = fopen("time_server.log", "a");
        if (fp != NULL){
            fprintf(fp, "%s\nConnection from: %s\n", time_msg, address_buffer);
            fprintf(fp, "Received: %d bytes.\n", bytes_received);
            fprintf(fp, "%.*s", bytes_received, request);
            fclose(fp);
        }
        
        SSL_shutdown(ssl);
        close(socket_client);
        SSL_free(ssl);       
    }

    close(socket_listen);
    SSL_CTX_free(ctx);
    
    return 0;
    

}