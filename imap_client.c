#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "string.h"

#include "imap_client.h"

#include "server_response.h"

#include "utils.h"

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
    // could change to the respnse instead 
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
    //printf("Server Response: %s\n\n", buffer);

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
    // snprintf(command, BUFFER_SIZE, "A02 SELECT %s\r\n", folder_name);

    // Escape double quotes and backslashes in folder_name
    char escaped_name[2*BUFFER_SIZE]; // Allow for potential doubling in size
    int i, j = 0;
    for (i = 0; folder_name[i] != '\0'; i++) {
        if (folder_name[i] == '"' || folder_name[i] == '\\') {
            escaped_name[j++] = '\\';
        }
        escaped_name[j++] = folder_name[i];
    }
    escaped_name[j] = '\0'; // Null-terminate the escaped string

    // Construct SELECT command with quoted and escaped folder name
    snprintf(command, BUFFER_SIZE, "A02 SELECT \"%s\"\r\n", escaped_name);

    // send the command to server
    send_command(sockfd, command); 

    // read the response
    read_response(sockfd, "A02");
}




void retrieve(int sockfd, int message_num, list_t *packet_list) {
    // The command is retrieve, we need to fetch the email:
    // tag FETCH messageNum BODY.PEEK[]

    // if messageNum = -1, it means the message sequence number was not specified on the command line 
    // fetch the last added message in the folder 

    char command[BUFFER_SIZE];

    if (message_num == -1) {
        // Fetch the last message (assuming UIDNEXT is available form SELECT)
        snprintf(command, BUFFER_SIZE, "A03 FETCH * BODY.PEEK[]\r\n");
    } else {
        // Fetch a specific message with messageNum
        snprintf(command, BUFFER_SIZE, "A03 FETCH %d BODY.PEEK[]\r\n", message_num);
    }

    // Now we sent the FETCH command
    send_command(sockfd, command); 


    // Read the response and store packets in a linked list 
    //list_t *packet_list = make_empty_list(); // create an empty list 
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int numBytes = read(sockfd, buffer, BUFFER_SIZE -1); 
        if (numBytes < 0) {
            error("ERROR reading from socket", 1);
        }
        buffer[numBytes] = '\0';

        // Check for lines to exclude
        // if (strstr(buffer, "A03 OK") || strstr(buffer, "* FETCH")) {
        //     continue; // Skip this line and read the next one
        // }


        // Check for continuation ("+") response
        if (buffer[0] == '+') {
            // More data is coming, store what we have so far
            insert_at_foot(packet_list, buffer + 1); // Skip the "+"
        } else {
            // Not a continuation, store the entire line
            insert_at_foot(packet_list, buffer); 
        }

        // Check for tagged response
        if (strstr(buffer, "A03 OK")) {
            break;  // Exit the loop after storing the last packet
        }

        // Check for message not found error
        if (strstr(buffer, "A03 NO")) {
            printf("Message not found\n");
            free_list(packet_list); // Free the list before exiting
            exit(3);  // Exit with status 3
        }

        if (strstr(buffer, "A03 BAD")) {
            printf("Message not found\n");
            free_list(packet_list); // Free the list before exiting
            exit(3);  // Exit with status 3
        }

        // Prints out every packet
        // will contain the raw content of the email message fetched, including headers, body and MIME structure 
        // printf("\n\n\n\nEvery packet:\n");
        // printf("%s", buffer);
        

    }

    // printf("\n \n\n-----------------PRINTING THE LIST -----------\n");
    // print_list_retrieve(packet_list); // Or iterate through the list using head and next pointers

    // // Free the linked list
    // free_list(packet_list);
    

}


void mime(int sockfd, list_t *packet_list) {

    
    // Concatenate all packets into a single buffer 
    char *buffer = concatenate_packets(packet_list); 
    //printf("%s", buffer); 

    char *boundary = find_mime_boundary(buffer);
    printf("\nboundary: %s \n", boundary);

    // Now find the MIME boundary 
    //char *boundary = 


    free(buffer); 

}


// Helper function to concatenate packets into a single buffer 
char *concatenate_packets(list_t *packet_list) {
    // calcualte the total size needed
    size_t total_size = 0; 
    
    node_t *current = packet_list ->head; 

    while (current != NULL) {
        total_size += strlen(current->packet); 
        current = current->next; 

    }
    // Allocate memory for the concatenated buffer
    char *buffer = (char *)malloc(total_size + 1); 
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1); 
    }

    // Copy packet data into the buffer
    buffer[0] = '\0';
    current = packet_list->head; 
    while(current != NULL) {
        strcat(buffer, current->packet); 
        current = current->next; 
    }

    return buffer; 
}


// Locate and extract the boundary parameter from MIME headers 
char *find_mime_boundary(const char *content) {

    
    // Search for the MIME-Version header (case-insensitive)
    char *mime_start = strcasestr(content, "mime-version: 1.0");
    if (!mime_start) {
        return NULL;
    }


    //printf("mime_start: %s", mime_start);

    // // Serach for the content-type header with boundary parameter
    // char *boundary_start = strstr(mime_start, "boundary=\"");
    // if (!boundary_start) return NULL; 

    // // move poitner to the start of the boundary value
    // boundary_start += strlen("boundary=\""); 
    
    // // Find the end of the boundary value (the closing double quote)
    // char *boundary_end = strchr(boundary_start, '\"');
    // if (!boundary_end) return NULL;

    // // Temporarily replace the closing quote with a null terminator to isolate the boundary value   
    // *boundary_end = '\0';
    // return boundary_start;

    // Search for the Content-Type header with boundary parameter (case-insensitive)
    char *boundary_start = strcasestr(mime_start, "boundary=");
    if (!boundary_start) {
        return NULL;
    }

    // Move pointer to the start of the boundary value
    boundary_start += strlen("boundary=");

    // Handle potential double quotes around the boundary value
    char *boundary_end;
    if (*boundary_start == '\"') {
        boundary_start++;
        boundary_end = strchr(boundary_start, '\"');
        if (!boundary_end) {
            return NULL;
        }
    } else {
        // In case boundary value is not quoted
        boundary_end = strpbrk(boundary_start, ";\n");
        if (!boundary_end) {
            boundary_end = boundary_start + strlen(boundary_start);
        }
    }

    // Calculate the length of the boundary value
    size_t boundary_length = boundary_end - boundary_start;

    // Allocate memory for the actual boundary value
    char *boundary = (char *)malloc(boundary_length + 1);
    if (!boundary) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    // Copy the boundary value into the allocated memory
    strncpy(boundary, boundary_start, boundary_length);
    boundary[boundary_length] = '\0';

    return boundary;

}



