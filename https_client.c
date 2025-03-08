#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h> 
#include <openssl/ssl.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/err.h>


int main(int argc, char *argv[]){

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        fprintf(stderr, "Call to SSL_CTX_new() failed.\n");
        return 1;
    }

    // add certificate verification


    if (argc < 3) {
        fprintf(stderr, "usage: %s hostname port\n", argv[0]);
        return 1;
    }

    char *hostname = argv[1];
    char *port = argv[2];

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(hostname, port, &hints, &peer_address)){
        fprintf(stderr, "Call to getaddrinfo() failed. (%d)\n",errno);
        return 1;
    }

    int server;
    server = socket(peer_address->ai_family, peer_address->ai_socktype, peer_address->ai_protocol);
    if (server < 0){
        fprintf(stderr, "Call to socket() failed. (%d)\n", errno);
        return 1;
    }

    if (connect(server, peer_address->ai_addr, peer_address->ai_addrlen)){
        fprintf(stderr, "Call to connect() failed. (%d)\n", errno);
        return 1;
    }

    freeaddrinfo(peer_address);

    printf("Connected.\n\n");

    SSL *ssl = SSL_new(ctx);
    if (!ctx) {
        fprintf(stderr, "Call to SSL_NEW() failed.\n");
        return 1;
    }

    if (!SSL_set_tlsext_host_name(ssl, hostname)){
        fprintf(stderr, "Call to SSL_set_tlsext_host_name() failed.\n");
        ERR_print_errors_fp(stderr);
        return 1;
    }

    SSL_set_fd(ssl,server);
    if (SSL_connect(ssl) == -1) {
        fprintf(stderr, "Call to SSL_connect() failed.\n");
        ERR_print_errors_fp(stderr);
        return 1;
    }

    printf("SSL/TLS using %s\n", SSL_get_cipher(ssl));

    X509 *cert = SSL_get_peer_certificate(ssl);
    if (!cert){
        fprintf(stderr, "Call to SSL_get_peer_certificate() failed.\n");
        return 1;
    }

    char *tmp;
    if ((tmp = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0))){
        printf("subject: %s\n", tmp);
        OPENSSL_free(tmp);
    }
    if ((tmp = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0))){
        printf("issuer: %s\n", tmp);
        OPENSSL_free(tmp);
    }

    X509_free(cert);

    char buffer[2048];

    sprintf(buffer, "GET / HTTP/1.1\r\n");
    sprintf(buffer + strlen(buffer), "Host: %s:%s\r\n", hostname, port);
    sprintf(buffer + strlen(buffer), "Connection: close\r\n");
    sprintf(buffer + strlen(buffer), "User-Agent: me\r\n");
    sprintf(buffer + strlen(buffer), "\r\n");

    SSL_write(ssl, buffer, strlen(buffer));
    printf("Sent Headers:\n%s", buffer);

    while(1){
        int bytes_received = SSL_read(ssl, buffer, sizeof(buffer));
        if (bytes_received < 1){
            printf("\nConnection closed by peer.\n");
            break;
        }

        printf("Received (%d bytes): '%.*s'\n", bytes_received, bytes_received, buffer);

    }

    SSL_shutdown(ssl);
    close(server);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    
    return 0;
}