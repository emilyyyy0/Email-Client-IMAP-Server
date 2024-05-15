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
        fprintf(stderr, "error: getaddresinfo\n");
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

    //printf("command being sent to server: %s", cmd);

    // write system call transmits the command string to the server
    // sockfd: Specifies the file descriptor of the socket, which identifies the connection to the server.
    // cmd: the command string to be sent
    // len: tell write() the exact number of bytes to send from the buffer

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


// Function to log into the server
void login(int sockfd, const char* username, const char* password) {
    char command[BUFFER_SIZE]; // Buffer to hold the complete command string

    // Format the login command according to the IMAP protocol
    snprintf(command, sizeof(command), "A01 LOGIN %s %s\r\n", username, password); 

    // send the login command to the server
    send_command(sockfd, command); 

    // Read the response to the login command
    read_response(sockfd, "A01"); // pass tag used in the login command
}

// Function to select a folder we want to read from 
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



// If command is retrieve, we fetch the email
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


        // Check for continuation ("+") response
        if (buffer[0] == '+') {
            // More data is coming, store what we have so far
            insert_at_foot(packet_list, buffer + 1, NULL); // Skip the "+"
        } else {
            // Not a continuation, store the entire line
            insert_at_foot(packet_list, buffer, NULL); 
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

    }

    

}


// function that decode MIME messages
void mime(int sockfd, list_t *packet_list) {
    
    // Concatenate all packets into a single buffer 
    char *buffer = concatenate_packets(packet_list); 
    //printf("%s", buffer); 

    // Now find the MIME boundary
    char *boundary = find_mime_boundary(buffer);
    //printf("\n%s\n", boundary);

    // Now we have to parse and decode the MIME parts

    // Content-Type: text/plain with charset=UTF-8
    // Content-Transfer-Encoding: quoted-printable | 7 bit | 8 bit
    
    parse_mime_parts(buffer, boundary); 

    free(boundary); 
    free(buffer); 
    return;

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


// Function to parse and process MIME parts
void parse_mime_parts(const char *email_content, const char *boundary) {
    // initial setup and finding the first boundary
    char delimiter[256];
    snprintf(delimiter, sizeof(delimiter), "--%s", boundary);

    // Find the first boundary delimiter
    char *part = strstr(email_content, delimiter);
    if (!part) {
        printf("Error: Boundary delimiter not found\n");
        return;
    }
    // part = contains everything from the boundary including the boundary itself

    // Process each MIME part
    while (part) {
        // Skips past the boundary
        part += strlen(delimiter);
        if (*part == '-' && *(part + 1) == '-') {
            break;  // Reached the end of the MIME parts
        }

        //printf("First PART: %s\n", part);
        unfold_headers_mime(part); 

        //printf("after unfolding: %s\n", part);
        

        // // Move to the next CRLF after the boundary delimiter
        // part += 2;  // Move past the CRLF
        part = strstr(part, "\n");
        if (!part) part = strstr(part, "\r\n");
        if (!part) break;
        part += (part[0] == '\r') ? 2 : 1;  // Move past the line ending

        //printf("Second PART: %s\n", part);

        // Find the headers end (empty line)
        char *headers_end = strstr(part, "\r\n\r\n");
        if (!headers_end) break;

        // Extract headers
        char headers[1024];
        strncpy(headers, part, headers_end - part);
        headers[headers_end - part] = '\0';
        //printf("Headers: %s\n", headers);  // Debug print

        // Move to the body part
        part = headers_end + 4;

        // Check Content-Type and Content-Transfer-Encoding headers
        char *content_type = NULL;
        char *encoding = NULL;
        parse_headers(headers, &content_type, &encoding);

        //printf("the content-type: %s, encoding: %s\n", content_type, encoding);
        // Validate headers
        if (!content_type || !strcasestr(content_type, "text/plain") || !strcasestr(content_type, "charset=UTF-8")) {
            printf("Error: Invalid Content-Type or charset\n");
            exit(4);
        }

        if (!encoding || !(strcasestr(encoding, "quoted-printable") || strcasestr(encoding, "7bit") || strcasestr(encoding, "8bit"))) {
            printf("Error: Invalid Content-Transfer-Encoding\n");
            exit(4);
        }
        

        // Process the first text/plain part with charset=UTF-8
        if (content_type && strcasestr(content_type, "text/plain") && strcasestr(content_type, "charset=UTF-8")) {
            char body[10240];  // Adjust size if necessary
            
            strncpy(body, part, sizeof(body) - 1);
            body[sizeof(body) - 1] = '\0';

            char *result = get_body_up_to_boundary(part, boundary); // Cut the part into just the part we want
            printf("%s", result);
           

            //printf("PRINTING THE BODY\n");
            // Print the body up to the next boundary delimiter
            //print_body_up_to_boundary(body, boundary);
            free(result); 
            return;  // Stop after printing the first matching part
        }

        // Find the next boundary delimiter
        part = strstr(part, delimiter);
    }

    return;
}

// Helper function to print the body up to the next boundary delimiter
void print_body_up_to_boundary(const char *body, const char *boundary) {
    char delimiter[256];
    snprintf(delimiter, sizeof(delimiter), "--%s", boundary);

    char *body_end = strstr(body, delimiter);
    if (body_end) {
        // Print up to the boundary delimiter
        fwrite(body, 1, body_end - body, stdout);
    } else {
        // Print the entire body if no boundary delimiter is found
        printf("%s", body);
    }
}





// Function to parse headers and extract values for Content-Type and Content-Transfer-Encoding
void parse_headers(const char *headers, char **content_type, char **encoding) {
    const char *current = headers;
    while (*current) {
        if (strncasecmp(current, "Content-Type:", 13) == 0) {
            *content_type = (char *)current + 13;
            while (**content_type == ' ' || **content_type == '\t') (*content_type)++;
        } else if (strncasecmp(current, "Content-Transfer-Encoding:", 26) == 0) {
            *encoding = (char *)current + 26;
            while (**encoding == ' ' || **encoding == '\t') (*encoding)++;
        }
        // Move to the next line
        current = strstr(current, "\r\n");
        if (!current) break;
        current += 2;
        // Handle folded headers (lines starting with space or tab)
        if (*current == ' ' || *current == '\t') {
            continue;
        }
    }
}


// Helper function to return the body up to the line before the next boundary delimiter as a string
char *get_body_up_to_boundary(const char *body, const char *boundary) {
    char delimiter[256];
    snprintf(delimiter, sizeof(delimiter), "--%s", boundary);

    // Find the boundary delimiter in the body
    char *body_end = strstr(body, delimiter);

    // Calculate the length of the body to return
    size_t body_length;
    if (body_end) {
        // Backtrack to find the newline before the boundary
        char *prev_newline = body_end;
        while (prev_newline > body && *(prev_newline - 1) != '\n') {
            prev_newline--;
        }

        // If there's a \r before \n, include it in body_length
        if (prev_newline > body && *(prev_newline - 2) == '\r') {
            prev_newline -= 2;
        } else if (prev_newline > body && *(prev_newline - 1) == '\n') {
            prev_newline--;
        }

        // Set body_length to be up to the line before the boundary
        body_length = prev_newline - body;
    } else {
        body_length = strlen(body);
    }

    // Allocate memory for the resulting string
    char *result = (char *)malloc(body_length + 1);
    if (!result) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Copy the body content up to the calculated length
    strncpy(result, body, body_length);
    result[body_length] = '\0'; // Null-terminate the string

    return result;
}



void unfold_headers_mime(char *headers) {
    int read_pos = 0, write_pos = 0;
    int len = strlen(headers);

    int content_type_pos = -1;
    int content_transfer_encoding_pos = -1;

    // Find positions of "Content-Type" and "Content-Transfer-Encoding" headers
    char *content_type_ptr = strstr(headers, "Content-Type:");
    if (content_type_ptr) {
        content_type_pos = content_type_ptr - headers;
    }

    char *content_transfer_encoding_ptr = strstr(headers, "Content-Transfer-Encoding:");
    if (content_transfer_encoding_ptr) {
        content_transfer_encoding_pos = content_transfer_encoding_ptr - headers;
    }

    // Determine the position up to which we need to unfold
    int unfold_pos = len;
    if (content_type_pos != -1 && content_transfer_encoding_pos != -1) {
        unfold_pos = (content_type_pos > content_transfer_encoding_pos) ? content_type_pos : content_transfer_encoding_pos;
    } else if (content_type_pos != -1) {
        unfold_pos = content_type_pos;
    } else if (content_transfer_encoding_pos != -1) {
        unfold_pos = content_transfer_encoding_pos;
    }

    // Loops through the string and unfold headers up to the determined position
    while (read_pos < unfold_pos && read_pos + 2 < len) {
        if (headers[read_pos] == '\r' && headers[read_pos + 1] == '\n' &&
            (headers[read_pos + 2] == ' ' || headers[read_pos + 2] == '\t')) { 
            read_pos += 2;
            while (read_pos < len && (headers[read_pos] == ' ' || headers[read_pos] == '\t')) {
                read_pos++;
            }
        } else {
            headers[write_pos++] = headers[read_pos++];
        }
    }

    // Copy any remaining characters that were not folded up to the determined position
    while (read_pos < len) {
        headers[write_pos++] = headers[read_pos++];
    }

    // Null-terminate the string
    headers[write_pos] = '\0';
}




// Parse
void parse(int sockfd, int message_num) {

    //list_t *header_list = make_empty_list(); 

    char command[BUFFER_SIZE];

    // Command construction based on if message num is given or not
    if (message_num == -1) {
        snprintf(command, BUFFER_SIZE, "A04 FETCH * BODY.PEEK[HEADER.FIELDS (FROM TO DATE SUBJECT)]\r\n");
    } else {
        snprintf(command, BUFFER_SIZE, "A04 FETCH %d BODY.PEEK[HEADER.FIELDS (FROM TO DATE SUBJECT)]\r\n", message_num);
    }

    send_command(sockfd, command);

    // Initial memory alloc of dynamic buffer
    char *dynamic_buffer = malloc(BUFFER_SIZE / 2);
    if (!dynamic_buffer) {
        error("Memory alloc failed\n", 1);
        exit(3);
    }
    dynamic_buffer[0] = '\0';

    // Initialise variable for reading in server response to dynamic buffer
    size_t total_bytes = 0;
    char read_buffer[BUFFER_SIZE];
    int num_bytes; // Num bytes read per while loop iteration

    // Read and store while read() is working
    while ((num_bytes = read(sockfd, read_buffer, BUFFER_SIZE - 1)) > 0) {
        read_buffer[num_bytes] = '\0';
        // Realloc dynamic buffer to size required after read() call
        char *new_buffer = realloc(dynamic_buffer, total_bytes + num_bytes + 1);
        // Error check if realloc fails
        if (!new_buffer) {
            error("Memory Alloc failed\n", 1);
            free(dynamic_buffer);
            exit(3);
        }
        // Reassigning dynamic buffer with the new bigger size and new data
        dynamic_buffer = new_buffer;
        memcpy(dynamic_buffer + total_bytes, read_buffer, num_bytes + 1);
        total_bytes += num_bytes;

        // If the end of headers has been reached stop reading
        if (strstr(dynamic_buffer, "\r\n\r\n"))
            break;
    }

    // Check if read() fails to read anything at all
    if (num_bytes < 0) {
        error("Unable to read from socket\n", 1);
        free(dynamic_buffer);
        exit(3);
    }

    // unfold_headers(dynamic_buffer);

    //printf("%s\n", dynamic_buffer);

    // Parse the headers from the dynamic buffer
    char date[MAX_HEADER_SIZE], from[MAX_HEADER_SIZE], to[MAX_HEADER_SIZE], subject[MAX_HEADER_SIZE];
    parse_headers_parse(dynamic_buffer, date, from, to, subject);

    // Print the parsed headers
    printf("From:%s\n", from);
    printf("To:%s\n", to);
    printf("Date:%s\n", date);
    printf("Subject:%s\n", subject);

    // Free the dynamic buffer
    free(dynamic_buffer);


    // the buffer now has everything 


    // We need: 
    // FROM
    // TO
    // DATE
    // SUBJECT

    // headers are case insensitive

}


void unfold_header(char *header) {
    char *src = header, *dst = header;
    while (*src) {
        if (*src == '\r' && *(src + 1) == '\n' && (*(src + 2) == ' ' || *(src + 2) == '\t')) {
            src += 2; // Skip CRLF
            while (*src == ' ' || *src == '\t') {
                src++; // Skip the folding whitespace
            }
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0'; // Null-terminate the unfolded header
}

void trim_whitespace(char *str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    end[1] = '\0';
}


void parse_headers_parse(const char *buffer, char *date, char *from, char *to, char *subject) {
    char line[MAX_HEADER_SIZE];
    const char *p = buffer;

    date[0] = from[0] = to[0] = subject[0] = '\0';
    strcpy(subject, " <No subject>");

    char *current_header = NULL;
    size_t current_length = 0;

    while (*p) {
        const char *line_start = p;
        while (*p && *p != '\n') p++;
        int line_length = p - line_start;

        if (*p == '\n') p++; // Move past the newline

        if (line_length > 0 && line_start[line_length - 1] == '\r') {
            line_length--; // Remove the CR character
        }

        strncpy(line, line_start, line_length);
        line[line_length] = '\0';

        // If the line is a continuation of the previous header (starts with space or tab)
        if (current_header && (line[0] == ' ' || line[0] == '\t')) {
            if (current_length + line_length < MAX_HEADER_SIZE) {
                strcat(current_header, line);
                current_length += line_length;
            }
        } else {
            // Process the previous header if we have one
            if (current_header) {
                unfold_header(current_header);
                trim_whitespace(current_header);
                current_header = NULL;
            }

            // Start a new header
            if (strncasecmp(line, "Date:", 5) == 0) {
                current_header = date;
                strcpy(date, line + 5);
                current_length = strlen(date);
            } else if (strncasecmp(line, "From:", 5) == 0) {
                current_header = from;
                strcpy(from, line + 5);
                current_length = strlen(from);
            } else if (strncasecmp(line, "To:", 3) == 0) {
                current_header = to;
                strcpy(to, line + 3);
                current_length = strlen(to);
            } else if (strncasecmp(line, "Subject:", 8) == 0) {
                current_header = subject;
                strcpy(subject, line + 8);
                current_length = strlen(subject);
            }
        }
    }

    // Process the last header if we have one
    if (current_header) {
        unfold_header(current_header);
        trim_whitespace(current_header);
    }
}



// A function to handle the List command
void list(int sockfd, int message_num, list_t *subject_list) {
    char command[BUFFER_SIZE];
    // Formation of command to get subject of each email
    snprintf(command, BUFFER_SIZE,
             "A06 FETCH 1:* (BODY[HEADER.FIELDS (SUBJECT)])\r\n");

 
    //flush_socket_buffer(sockfd); // Clear the buffer before sending new command
    send_command(sockfd, command);

    // Initial memory alloc for dynamic buffer with check
    char *dynamic_buffer = malloc(BUFFER_SIZE);
    if (!dynamic_buffer) {
        error("Memory allocation failed", 1);
        exit(3);
    }

    // Initialisation of varaibles for reading in the server response into
    // dynamic_buffer as a long string
    int buffer_len = BUFFER_SIZE;
    char read_buffer[BUFFER_SIZE]; // Buffer for currently read amount in that
                                   // while loop iteration
    int num_bytes = 0;             // Num bytes read in the current while loop
    int total_bytes = 0;

    // While loop runs until the end of response is reached or socket is closed
    while ((num_bytes = read(sockfd, read_buffer, BUFFER_SIZE - 1)) > 0) {
        // If buffer needs to be resized
        if (total_bytes + num_bytes >= buffer_len - 1) {
            buffer_len *=
                2; // Double the buffer size when approaching the limit
            char *new_buffer = realloc(dynamic_buffer, buffer_len);
            // Check if realloc fails
            if (!new_buffer) {
                error("Memory reallocation failed", 1);
                free(dynamic_buffer);
                exit(3);
            }
            // Assign resized buffer to dynamic buffer
            dynamic_buffer = new_buffer;
        }
        // Copy data to dynamic buffer and update counters
        memcpy(dynamic_buffer + total_bytes, read_buffer, num_bytes);
        total_bytes += num_bytes;
        dynamic_buffer[total_bytes] = '\0'; // Null-terminate the dynamic buffer

        // Check for tagged response to determine when to stop reading
        if (strstr(dynamic_buffer, "A06 OK") ||
            strstr(dynamic_buffer, "A06 NO") ||
            strstr(dynamic_buffer, "A06 BAD")) {
            break;
        }
    }
    // Error check if read() failed to read anything
    if (num_bytes < 0) {
        error("Unable to read from socket\n", 1);
        free(dynamic_buffer);
        exit(3);
    }
    // Check
   

    // Code to read in the server response and store in the subject_list with
    // correct format
    char *line = strtok(
        dynamic_buffer,
        "\r\n"); // Defines a line as all the text until "\r\n" is encountered
    char seq_num[20] =
        "<No sequence>"; // Buffer to store message seuqence number
    char subject[BUFFER_SIZE] = "<No subject>"; // Buffer to store the subject
    int subject_found = 0; // Variable to indicate if a subject has been found
    int first_message = 1; // Variable to handle the first message separately

    // Reads while there is a valid line to be read and stored
    while (line) {
        // Check for completion tags and errors
        if (strstr(line, "A06 OK Fetch completed") || strstr(line, "A06 NO") ||
            strstr(line, "A06 BAD")) {
         
            break; // Exit the loop if the end of the response or an error is
                   // encountered
        }

        // Check if the line starts with "* ", indicating the start of a new
        // message
        if (strncmp(line, "* ", 2) == 0) {
            // If a new message is being processed and its not the very first
            // message processed
            if (!first_message) {
                // If no subject was found for the previous message, insert "<No
                // subject>"
                if (!subject_found) {
                    insert_at_foot(subject_list, "<No subject>", seq_num);
                }
                // Otherwise add the subject in that is present
                else {
                    trim_subject(subject); // Trim the subject if necessary
                    // Check if the last character is ')', and remove it before
                    // storing
                    size_t len = strlen(subject);
                    if (len > 0 && subject[len - 1] == ')') {
                        subject[len - 1] = '\0';
                    }
                    insert_at_foot(subject_list, subject,
                                   seq_num); // Insert the subject into the list
                }
            }
            first_message =
                0; // Clear the first message variable after the first iteration

            // Extract the sequence number from the line
            sscanf(line, "* %s FETCH", seq_num);

            // Reset subject and flag for the new message
            strcpy(subject, "<No subject>");
            subject_found = 0;
        }

        // If it isn't the start of a new message then just add the rest of the
        // line to subject
        else if (subject_found && (line[0] != '*' && line[0] != '\0')) {
            // Append additional lines to the subject without leading space
            strncat(subject, line, BUFFER_SIZE - strlen(subject) - 1);
        }

        // Check if the line contains "Subject: "
        char *subject_start = strstr(line, "Subject: ");
        // Store the subject now, after storing the message sequence number
        if (subject_start) {
            subject_start += 9; // Skip past "Subject: "
            strncpy(subject, subject_start,
                    BUFFER_SIZE - 1);        // Copy the subject content
            subject[BUFFER_SIZE - 1] = '\0'; // Ensure null-termination

            // Remove trailing newlines and spaces from the subject
            trim_subject(subject);

            // Mark that the subject was found
            subject_found = 1;
        }

        // Move to the next line
        line = strtok(NULL, "\r\n");

        // Check if the next line indicates the start of a new message or end of
        // response
        if ((line && strncmp(line, "* ", 2) == 0) ||
            (line && strstr(line, "A06 OK Fetch completed"))) {
            // Trim appropriately before moving onto the next message
            trim_subject(subject);
        }
    }

    // Handle the last message if no subject was found
    if (!first_message) {
        if (!subject_found) {
            insert_at_foot(subject_list, "<No subject>", seq_num);
        } else {
            trim_subject(subject); // Trim the subject if necessary
            // Check if the last character is ')', and remove it before storing
            size_t len = strlen(subject);
            if (len > 0 && subject[len - 1] == ')') {
                subject[len - 1] = '\0';
            }
            insert_at_foot(subject_list, subject,
                           seq_num); // Insert the subject into the list
        }
    }

    // Code to now sort the subject_list and then print it
    sort_subject_list(subject_list);

    print_subject_list(subject_list);

    free(dynamic_buffer);
}




// Function to trim the last ")" at the end of each email subject 
void trim_subject(char *subject) {
    // Remove trailing newline or space characters from the subject
    char *end = subject + strlen(subject) - 1; // Grabs the end of subject
    // Looks for all the undesired character sand replaces with null terminating
    while (end > subject && (*end == '\r' || *end == '\n' || *end == ' ')) {
        *end = '\0';
        end--;
    }
}