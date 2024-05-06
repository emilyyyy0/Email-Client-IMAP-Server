#include <stdio.h>
#include <stdlib.h>

#include "imap_client.h"

// Implement functions to connect to the imap server using sockets
// Functions to log in, select folder, fetch messages, and other IMAP commands.
// Handle IMAP protocol specifics like tag generation and response parsing.

int create_connection(const char *hostname, const char *port) {
    struct addrinfo hints, *res, *res0; 
    // hints: used to provide criteria for selecting the desired socket address structures.
    // *res0: points to a linked list of address structures
    // res: temporary pointer used for iterating through the *res list
    int sockfd; // holds file decriptor of the socket

    memset(&hints, 0, sizeof(hints)); // initialises hints structure to all zeros
    hints.ai_family = AF_UNSPEC; // Allows IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // we want a stream socket (TCP)

    // Now we create a linked list of struct addrinfo each containing address information that can be used to create a socket
    // with res0 pointing to the head of the linked list 
    // the hints structure guides the type of addresses returned
    if (getaddrinfo(hostname, port, &hints, &res0) != 0) {
        fprintf(stderr, "error: getaddreinfo\n");
        exit(1); 
    }


    for (res = res0; res; res = res->ai_next) {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        // create socket using address family, socket type and protocol specified in res (current addrinfo struct)
        // If successful, sockfd will hold a non-negative file descriptor for the socket.
        
        if (sockfd < 0) {
            continue;
        }

        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0) {
            break; // successful connection to the address specified by the res (current addrinfo struct)
            // loop breaks, we have connected the socket. 
        }

        close(sockfd); // if connection attempt fails, socket is closed
        // continue looping to try the next address in the list. 

    }

    // Now we do some error handling
    if (res == NULL) { 
        fprintf(stderr, "Could not connect\n"); 
        exit(1); 
        // no address succeeded
    }

    freeaddrinfo(res0); 
    return sockfd; 

    printf("The connection function\n\n\n\n\n");
}

// const char *hostname, const char *port