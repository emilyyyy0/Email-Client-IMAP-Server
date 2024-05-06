#include <stdio.h>
#include <stdlib.h>

#include "imap_client.h"

// Implement functions to connect to the imap server using sockets
// Functions to log in, select folder, fetch messages, and other IMAP commands.
// Handle IMAP protocol specifics like tag generation and response parsing.

int create_connection(const char *hostname, const char *port) {
    struct addrinfo hints, *res, *res0; 

    printf("The connection function\n\n\n\n\n");
    return 1; 
}

// const char *hostname, const char *port