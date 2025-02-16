/*

DNS MESSAGE 

---------------------------
|         HEADER          |    <---  Information about message 
---------------------------
|        QUESTION         |    <---  Question for the name server
---------------------------
|         ANSWER          |    <---  Answer(s) to the question
---------------------------
|        AUTHORITY        |    <---  Pointers to other name servers 
---------------------------
|       ADDITIONAL        |    <---  Additional information 
---------------------------



DNS HEADER 

-----------------------------------------------------------------------
| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
-----------------------------------------------------------------------
|                             ID                                      |        ID  - value identifying the message
-----------------------------------------------------------------------
|QR |     OP CODE   |AA |TC |RD |RA |      Z      |       RCODE       |         
-----------------------------------------------------------------------
|                           QDCOUNT                                   |        QDCOUNT - number of questions         
-----------------------------------------------------------------------
|                           ANCOUNT                                   |        ANCOUNT - number of answers
-----------------------------------------------------------------------
|                           NSCOUNT                                   |        NSCOUNT - number of nameservers
-----------------------------------------------------------------------
|                           ARCOUNT                                   |        ARCOUNT - additional information count
-----------------------------------------------------------------------

QR - 0 indicates DNS query, 1 indicates DNS response; OPCODE - type of query; AA - authoritative answer; TC - message was truncated; 
RD - recursion desired; RA - recursion available; Z - unused and should be set to 0; RCODE - DNS response error condition;  



DNS QUESTION


-----------------------------------------------------------------------
| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
-----------------------------------------------------------------------
|                                                                     |
|                             NAME                                    |        NAME - max 255 bytes. Broken into invidual lebels, each label up to 63 characters
|                                                                     |
-----------------------------------------------------------------------
|                            QTYPE                                    |        QTYPE - record type
-----------------------------------------------------------------------
|                            QCLASS                                   |        QCLASS - set to 1 to indicate the Internet
-----------------------------------------------------------------------




DNS ANSWER 


-----------------------------------------------------------------------
| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
-----------------------------------------------------------------------
|                                                                     |
|                             NAME                                    |        NAME - max 255 bytes. Broken into invidual lebels, each label up to 63 characters
|                                                                     |        If two MSB set, the remaining 14 bits are pointer to name
-----------------------------------------------------------------------
|                             TYPE                                    |        TYPE - record type
-----------------------------------------------------------------------
|                             CLASS                                   |        CLASS - set to 1 to indicate the Internet
-----------------------------------------------------------------------
|                              TTL                                    |        TTL - 32-bit field specifies how many seconds the answer is allowed to be cached for 
|                                                                     |
-----------------------------------------------------------------------
|                           RDLENGTH                                  |        RDLENGTH - data length
-----------------------------------------------------------------------
|                             RDATA                                   |        RDATA - data dependent on record type
-----------------------------------------------------------------------



*/


#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>



const unsigned char *print_name(const unsigned char *mssg, const unsigned char *p, const unsigned char *end);
void print_raw_dns_message(const char *message, int msg_length);
void print_dns_message(const char *message, int msg_length);



int main(int argc, char *argv[]){


    if (argc < 3){
        printf("Usage:\n\tfilename hostname type\n");
        exit(0);
    }

    if (strlen(argv[1]) > 255){
        fprintf(stderr,"Hostname too long.\n");
        exit(1);
    }

    unsigned char type; 
    if (strcmp(argv[2], "a") == 0){
        type == 1;
    } else if (strcmp(argv[2],"mx") == 0){
        type = 15;
    } else if (strcmp(argv[2], "txt") == 0){
        type = 16;
    } else if (strcmp(argv[2], "aaaa") == 0){
        type = 28;
    } else if (strcmp(argv[2], "any") == 0){
        type = 255;
    } else {
        fprintf(stderr, "Unknown type: '%s'. Use a, aaaa, mx, txt or any.", argv[2]);
        exit(1);
    }

    struct addrinfo hints; 
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype=SOCK_DGRAM;
    struct addrinfo *peer;
    if (getaddrinfo("8.8.8.8", "53", &hints, &peer)){
        fprintf(stderr, "call to getaddrinfo failed. (%d)\n", errno);
        return 1;
    }





    int socket_send; 
    if (socket_send = socket(peer->ai_family, peer->ai_socktype, peer->ai_protocol) < 0){
        fprintf(stderr, "call to socket() failed. (%d)\n", errno);
        return 1;
    }



    
    char query [1024] = {0x00, 0x45,
                0x01, 0x00,
                0x00, 0x01,
                0x00, 0x00,
                0x00, 0x00,
                0x00, 0x00,
                }; 
  
    char *p = query + 12;
    char *h = argv[1];

    while(*h){
        char *len = p; 
        p++;
        if (h != argv[1]) ++h;

        while(*h && *h != '.') *p++ = *h++;
        *len = p - len - 1;
    }

    *p++ = 0;

    *p++ = 0x00;
    *p++ = type; 
    *p++ = 0x00;
    *p++ = 0x01;

    const int query_size = p - query; 

    int bytes_sent = sendto(socket_send, query, query_size, 0, peer->ai_addr, peer->ai_addrlen);
    printf("Sent %d bytes. \n", bytes_sent);

    print_dns_message(query, query_size);

    char read[1024];
    int bytes_received = recvfrom(socket_send, read, 1024, 0, 0, 0);
    printf("Received %d bytes. \n", bytes_received);

    print_dns_message(read, bytes_received);
    printf("\n");

    freeaddrinfo(peer);
    close(socket_send);
    return 0;

}


const unsigned char *print_name(const unsigned char *mssg, const unsigned char *p, const unsigned char *end){

    if (p + 2 > end){
        fprintf(stderr, "End of message.\n");
        exit(1);
    }

    // check if the name is a pointer (2 MSB set)
    if ((*p & 0xC) == 0xC0){
        // store pointer (14 remaining bits)
        const int k = ((*p & 0x3F) << 8) + p[1];
        p += 2;
        printf(" (pointer %d)", k);

        // recursive call with pointer to the name 
        print_name(mssg, mssg+k, end);
        return p;
    } else{
        const int len = *p++; 
        if (p + len + 1 > end){
            fprintf(stderr, "End of message.\n");
            exit(1);
        }
        printf("%.*s", len, p);
        p += len; 
        if (*p){
            printf(".");
            return print_name(mssg, p, end);
        } else{
            return p+1; 
        }

    }
}


void print_raw_dns_message(const char *message, int msg_length){
   
    // check if message is shorter than header length(12 bytes) 
    if (msg_length < 12){
        fprintf(stderr, "Message is too short to be valid.\n");
        exit(1);
    }

    const unsigned char *msg = (const unsigned char *)message;

    
    //  PRINT RAW MESSAGE  
    int i;
    for (i=0; i < msg_length; ++i){
        unsigned char r = msg[i];
        printf("%02d:   %02X  %03d  '%c'\n", i, r, r, r);
    }

      
}


void print_dns_message(const char *message, int msg_length){
   
    // check if message is shorter than header length(12 bytes) 
    if (msg_length < 12){
        fprintf(stderr, "Message is too short to be valid.\n");
        exit(1);
    }

    const unsigned char *msg = (const unsigned char *)message;

    
    // print first two bytes(ID)
    printf("ID = %0X %0X\n", msg[0], msg[1]);
   
    // check for QR bit 
    const int qr = (msg[2] & 0x80) >> 7;
    printf("QR = %d %s\n", qr, qr ? "response" : "query");

    const int opcode = (msg[2] & 0x78) >> 3;
    printf("OPCODE = %d ", opcode);
    switch(opcode){
        case 0: printf("standard\n"); break;
        case 1: printf("reverse\n"); break;
        case 2: printf("status\n"); break;
        default: printf("?\n"); break;
    }

    // print AA
    const int aa = (msg[2] & 0x04) >> 2;
    printf("AA = %d %s\n", aa, aa? "authoritative" : "");

    // print TC
    const int tc = (msg[2] & 0x02) >> 1;
    printf("TC = %d %s\n", tc, tc ? "message truncated" : "");

    // print RD
    const int rd = (msg[2] & 0x01);
    printf("RD = %d %s\n", rd, rd ? "recursion desired" : "");


    // print RCODE for response message
    if (qr){
        const int rcode = msg[3] & 0x07;
        printf("RCODE = %d ", rcode);
        switch(rcode){
            case 0: printf("success\n"); break;
            case 1: printf("format error\n"); break;
            case 2: printf("server failure\n"); break;
            case 3: printf("name error\n"); break;
            case 4: printf("not implemented\n"); break; 
            case 5: printf("refused\n"); break;
            default: printf("?\n"); break;
        }
        if (rcode != 0) return; 
    }

    const int qdcount = (msg[4] << 8) + msg[5];
    const int ancount = (msg[6] << 8) + msg[7];
    const int nscount = (msg[8] << 8) + msg[9];
    const int arcount = (msg[10] << 8) + msg[11];

    printf("QDCOUNT = %d\n", qdcount);
    printf("ANCOUNT = %d\n", ancount);
    printf("NSCOUNT = %d\n", nscount);
    printf("ARCOUNT = %d\n", arcount);


    const unsigned char *p = msg + 12; 
    const unsigned char *end = msg + msg_length;

    if (qdcount){
        int i;
        for(i = 0; i < qdcount; ++i){
            if (p >= end){
                fprintf(stderr, "End of message.\n");
                exit(1);
            }

            printf("Query %2d\n", i + 1);
            printf(" name: ");

            p = print_name(msg, p, end);
            printf("\n");

            if (p + 4 > end){
                fprintf(stderr, "End of message.\n");
                exit(1);
            }
        const int type = (p[0] << 8) + p[1];
        printf("  type: %d\n", type);

        const int qclass = (p[0] << 8) + p[1];
        printf("  class: %d\n", qclass);
        p += 2;
        }
    }

    if (ancount || nscount || arcount){
        int i; 
        for (i = 0; i < ancount + nscount + arcount; ++i){
            if (p >= end){
                fprintf(stderr, "End of message.\n");
                exit(1);
            }

            printf("Answer %2d\n", i + 1);
            printf("  name:  ");

            p = print_name(msg, p, end);
            printf("\n");

            if (p + 10 > end){
                fprintf(stderr, "End of message.\n");
                exit(1);
            }

            const int type = (p[0] << 8) + p[1];
            printf("   type: %d\n", type);
            p += 2; 

            const int qclass = (p[0] << 8) + p[1];
            printf("  class: %d\n", qclass);
            p += 2;

            const unsigned int ttl = (p[0] << 24) + (p[1] << 16) + (p[2] << 8) +p[3];
            printf("  ttl:  %u\n", ttl);
            p += 4;

            const int rdlen = (p[0] << 8) + p[1];
            printf("  rdlen: %d\n", rdlen);
            p += 2; 

            if (p + rdlen > end){
                fprintf(stderr, "End of message.\n");
                exit(1);
            }

            if (rdlen == 4 && type == 1){
                printf("Address ");
                printf("%d.%d.%d.%d\n", p[0], p[1], p[2], p[3]);
            } else if (rdlen > 3 && type == 15){
                const int preference = (p[0] << 8) + p[1];
                printf("  pref: %d\n", preference);
                printf("MX: ");
                print_name(msg, p+2, end); 
                printf("\n");
            } else if(rdlen == 16 && type == 28){
                printf("Address ");
                int j;
                for(j = 0; j < rdlen; j+=2){
                    printf("%02x%02x", p[j], p[j+1]);
                    if (j + 2 < rdlen) printf(":");
                }
                printf("\n");
            } else if(type == 16){
                printf("TXT: '%.*s'\n", rdlen-1, p+1);
            } else if(type == 5){
                printf("CNAME: ");
                print_name(msg, p, end);
                printf("\n");
            }
            p += rdlen;
        }

    }

    if ( p != end ){
        printf("There is some unread data left over. \n");
    }
    printf("\n");

}

