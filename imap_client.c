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
    int total_sent = 0;  // Total bytes sent so far
    int bytes_left = len;  // Bytes left to send
    int bytes_sent;

    printf("command being sent to server: %s", cmd);

    // write system call transmits the command string to the server
    // sockfd: Specifies the file descriptor of the socket, which identifies the connection to the server.
    // cmd: the command string to be sent
    // len: tell write() the exact number of bytes to send from the buffer
    // if (write(sockfd, cmd, len) != len) {
    //     // this ensures  that all bytes of the command string were successfully written to the socket
    //     error("socket is not writable", 1); // partial write or a write failure
    // }

     // Continuously attempt to write until all bytes are sent
    while (bytes_left > 0) {
        bytes_sent = write(sockfd, cmd + total_sent, bytes_left);
        if (bytes_sent < 0) {
            // Error handling
            perror("Failed to write to socket");
            exit(1); // Exit or handle error appropriately
        }
        total_sent += bytes_sent;  // Update total bytes sent
        bytes_left -= bytes_sent;  // Decrease the byte count remaining
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

    // Check for specific login failure response
    if (strstr(buffer, "A01 NO [AUTHENTICATIONFAILED]")) { 
        printf("Login failure\n");
        exit(3);  // Exit with status code 3
    }

    // Check if folder not found 
    if (strstr(buffer, "A02 NO")) {
        printf("Folder not found\n");
        exit(3);
    }

    // Output the server's response for debugging or information purposes
    printf("Server Response: %s\n\n", buffer);

}



void login(int sockfd, const char* username, const char* password) {
    char command[BUFFER_SIZE]; // Buffer to hold the complete command string

    // Format the login command according to the IMAP protocol
    snprintf(command, sizeof(command), "A01 LOGIN %s %s\r\n", username, password); 

    // send the login command to the server
    send_command(sockfd, command); 

    // Read the response to the login command
    read_response(sockfd, "A01"); // pass tag used in the login command
}


void select_folder(int sockfd, const char *folder_name) {
    char command[BUFFER_SIZE];

    // Use "INBOX" if folder_name is NULL or empty
    if (folder_name == NULL || strlen(folder_name) == 0) {
        folder_name = "INBOX";
    }

    // Construct SELECT command
    snprintf(command, BUFFER_SIZE, "A02 SELECT %s\r\n", folder_name);

    // send the command to server
    send_command(sockfd, command); 

    // read the response
    read_response(sockfd, "A02");
}



// const char *hostname, const char *port