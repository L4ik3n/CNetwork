/* GET NETWORK INTERFACES (UNIX-BASED) */


#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>

int main() {

    struct ifaddrs *addresses;

    // return linked list of interfaces (returns 0 on success, -1 on failure) 
    if (getifaddrs(&addresses) == -1) {
        printf("call failed \n");
        return -1; 
    }


    struct ifaddrs *address = addresses;

    // loop through linked list and print interface name, type(only v4 and v6) and address
    while(address) {
        int family = address->ifa_addr->sa_family;
        // check if interface type is either v4 or v6 
        if (family == AF_INET || family == AF_INET6) {
            printf("%s\t", address->ifa_name);
            printf("%s\t", family == AF_INET ? "IPv4": "IPv6");
            // define buffer to store the address in text 
            char addrBuffer[100];
            // define size of address to get 
            const int family_size = family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
            // get address
            getnameinfo(address->ifa_addr, family_size, addrBuffer, sizeof(addrBuffer), 0, 0, NI_NUMERICHOST);
            printf("\t%s\n", addrBuffer);

        }
        address = address->ifa_next;
    }

    // free memory 
    freeifaddrs(addresses);
    return 0;

}