// DOMAIN NAME RESOLUTION 


#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



int main (int argc, char *argv[]){


    // check for arguments 
    if (argc < 2){
        fprintf(stderr, "Usage: filename hostname\n");
        return 1;
    }

    // configure  address hints 
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_ALL;
    struct addrinfo *host_addr;

    // resolve domain name   
    if (getaddrinfo(argv[1], 0, &hints, &host_addr)){
        fprintf(stderr, "call to getaddinfo failed. (%d)\n", errno);
        return 1;
    }

    
    // print all addresses 
    struct addrinfo *address = host_addr;
    do{
        char next_address[100];
        char current_address[100];
        // get address
        getnameinfo(address->ai_addr, address->ai_addrlen, current_address, sizeof(current_address), 0, 0, NI_NUMERICHOST);
        // if there is next address get it 
        if (address->ai_next) getnameinfo(address->ai_next->ai_addr, address->ai_next->ai_addrlen, next_address, sizeof(next_address), 0, 0, NI_NUMERICHOST);
        else memset(next_address, 0, sizeof(next_address));   
        // if the next address is the same as current continue, else print current address 
        if (strcmp(current_address, next_address) == 0) continue;
        else printf("Address: %s\t%d %d\n ", current_address, address->ai_protocol, address->ai_socktype);

    } while (address = address->ai_next);
    

    // free memory 
    freeaddrinfo(host_addr);


    return 0; 



}