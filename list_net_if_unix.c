/* GET NETWORK INTERFACES (UNIX-BASED) */



#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct ifAddr{
    char* name;
    char* ipv4;
    char* ipv6;    
    struct ifAddr* next;
} ifAddr; 


// add first element to list
void addEmpty(ifAddr* head, char* name){
    head->name = strdup(name);
    head->ipv4 = NULL;
    head->ipv6 = NULL;
    head->next = NULL;


}

// push to the list 
void add(ifAddr* head, char* name){
    ifAddr * current = head;
    while (current->next != NULL){
        current = current->next;
    }

    current->next = (ifAddr *) malloc(sizeof(ifAddr));
    current->next->name = strdup(name);
    current->next->ipv4 = NULL;
    current->next->ipv6 = NULL;
    current->next->next = NULL;
    

}


// fill in ip values 
void addIp(ifAddr * head, char* name, char* ip, int family){
    
    ifAddr* current = head;
        while(current){
            if (!strcmp(current->name,name)){
                if (family == AF_INET)
                    current->ipv4 = strdup(ip);
                else if (family == AF_INET6)
                    current->ipv6 = strdup(ip);
            }   
            current = current->next;
        }
    
}

// loop through list to check if interface already exists (returns 0 if exists, 1 otherwise )

int searchList(ifAddr* head, char* name){
    ifAddr* current = head;
    while(current){
        if (!strcmp(current->name, name))
            return 0; 
        current = current->next;
    }
    return 1;


}


// free list memory 

void freeList(ifAddr * head)
{
    ifAddr * temp;
    while(head){
        temp = head;
        head = head->next;
        free(temp->name);
        free(temp->ipv4);
        free(temp->ipv6);
        free(temp);

    } 

}

int main() {

    
    ifAddr * list = NULL;
    list = (ifAddr *) malloc(sizeof(ifAddr));
    list->name = NULL;
    list->ipv4 = NULL;
    list->ipv6 = NULL;
    list->next = NULL;
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
            
            // check if list is empty
            if (list -> name){
                // check if interfaces already exists
                if (searchList(list, address->ifa_name))
                    add(list, address->ifa_name);
            }
            else
                addEmpty(list, address->ifa_name);
            // define buffer to store the address in text 
            char addrBuffer[100];
            // define size of address to get 
            const int family_size = family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
            // get address
            getnameinfo(address->ifa_addr, family_size, addrBuffer, sizeof(addrBuffer), 0, 0, NI_NUMERICHOST);
            addIp(list, address->ifa_name, addrBuffer, family);
        }
        address = address->ifa_next;
    }

    // print interfaces 
    ifAddr* temp = list; 
    while(temp){
        printf("%s\n", temp->name);
        if (temp->ipv4)
            printf("\tIPv4: %s\n", temp->ipv4);
        if (temp->ipv6)
            printf("\tIPv6: %s\n", temp->ipv6);

        temp = temp->next;
    }

    // free memory 
    freeList(list);

    freeifaddrs(addresses);
    return 0;

}

