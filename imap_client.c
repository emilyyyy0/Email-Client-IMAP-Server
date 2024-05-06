#include <stdio.h>
#include <stdlib.h>

#include "string.h"

#include "imap_client.h"

// Implement functions to connect to the imap server using sockets
// Functions to log in, select folder, fetch messages, and other IMAP commands.
// Handle IMAP protocol specifics like tag generation and response parsing.

void error(const char *msg, int num) {
    perror(msg);
    exit(num);
}

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

}

// send_command function 
// sends a string (command) to the server over the socket connection (represented by sockfd)
void send_command(int sockfd, const char *cmd) {
    int len = strlen(cmd);

    printf("command being sent to server: %s", cmd);

    // write system call transmits the command string to the server
    // sockfd: Specifies the file descriptor of the socket, which identifies the connection to the server.
    // cmd: the command string to be sent
    // len: tell write() the exact number of bytes to send from the buffer
    if (write(sockfd, cmd, len) != len) {
        // this ensures  that all bytes of the command string were successfully written to the socket
        error("socket is not writable", 1); // partial write or a write failure
    }
}


// now we have to read the response from the server
void read_response(int sockfd, const char* tag) {
    char buffer[BUFFER_SIZE]; // character array to store received data
    memset(buffer, 0, BUFFER_SIZE); // initialises buffer with zeros to ensure no residual data from previous
    int numBytes = read(sockfd, buffer, BUFFER_SIZE - 1); // Attempts to read data from the socket with file descriptor sockfd into the buffer. It reads a maximum of BUFFER_SIZE - 1 bytes to leave room for the null terminator (\0) that will be added later.
    if (numBytes < 0) {
        // if negative, an error occured during reading.
        error("ERROR reading from socket", 1);
    }

    buffer[numBytes] = '\0'; // null terminator to make it a string

    // Output the server's response for debugging or information purposes
    printf("Server Response: %s\n", buffer);

    // Now we check if response was successful 
    // char expected_ok[100];
    // snprintf(expected_ok, sizeof(expected_ok), "%s OK", tag);

    // if (strstr(buffer, expected_ok) != NULL) {
    //     printf("Command successful: %s\n", buffer);
    // } else {
    //     printf("Login failure\n");
    //     exit(3);
    // }


    //printf("%s", buffer); // prints receive response to stdout

}


// const char *hostname, const char *port