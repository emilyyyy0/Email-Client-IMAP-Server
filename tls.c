#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "tls.h"
#include "utils.h"


// Initialise OpenSSL 
// Create an SSL conext - to establish SSL connection 
// Load the certificate
// create and configure an SSL connection 


int tls_connect(const char *hostname, const char *port, SSL **ssl_out) {
    
    int sockfd; 

    struct hostent *server;
    struct sockaddr_in serv_addr;

    // Resolve the hostname to an IP address
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        return -1;
    }

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return -1;
    }

    // Zero out the server address structure and set the family and port
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(atoi(port)); // Convert port from string to integer

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        close(sockfd);
        return -1;
    }

    // Initialize OpenSSL libraries
    initialize_openssl();

    // Create an SSL context using the CA certificate
    SSL_CTX *ctx = create_ssl_context("/usr/local/share/ca-certificates/ca.crt");
    if (!ctx) {
        close(sockfd);
        return -1;
    }

    // Create an SSL connection using the context and socket file descriptor
    SSL *ssl = create_ssl_connection(ctx, sockfd);
    if (!ssl) {
        close(sockfd);
        SSL_CTX_free(ctx);
        return -1;
    }

    // Check the result of the certificate verification
    if (SSL_get_verify_result(ssl) != X509_V_OK) {
        fprintf(stderr, "Certificate verification failed\n");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }

    // Read the initial server response
    read_ssl_response(ssl);

    // Return the SSL pointer to the caller
    *ssl_out = ssl;

    return sockfd; // Return the socket file descriptor
}



// Function to read the initial server response
void read_initial_server_response(SSL *ssl) {
    char buffer[BUFFER_SIZE_TLS];
    int bytes;

    while ((bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes] = 0; // Null-terminate the buffer
        //printf("%s", buffer);
    }

    if (bytes < 0) {
        int err = SSL_get_error(ssl, bytes);
        if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
            perror("Error reading server response");
            ERR_print_errors_fp(stderr);
        }
    }
}




// Function to print the server response
void print_server_response(SSL *ssl) {
    char buffer[BUFFER_SIZE_TLS];
    int bytes;

    while ((bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes] = 0; // Null-terminate the buffer
        printf("%s", buffer);
    }

    if (bytes < 0) {
        perror("Error reading server response");
        ERR_print_errors_fp(stderr);
    }
}



// Function to send a command over an SSL connection
void send_ssl_command(SSL *ssl, const char *command) {
    int len = strlen(command);
    int bytes = SSL_write(ssl, command, len);
    if (bytes < 0) {
        error("ERROR writing to SSL connection", 2);
    } else if (bytes != len) {
        fprintf(stderr, "Partial write\n");
    }
}



// Function to read response from an SSL connection
void read_ssl_response(SSL *ssl) {
    char buffer[BUFFER_SIZE_TLS]; // Character array to store received data
    memset(buffer, 0, BUFFER_SIZE_TLS); // Initializes buffer with zeros to ensure no residual data

    int numBytes = SSL_read(ssl, buffer, BUFFER_SIZE_TLS - 1); // Attempts to read data from the SSL connection
    if (numBytes < 0) {
        // If negative, an error occurred during reading
        error("ERROR reading from SSL connection", 2);
    } else if (numBytes == 0) {
        // Immediate disconnect
        fprintf(stderr, "Server disconnected immediately\n");
        SSL_free(ssl);
        exit(2); // or exit(3), depending on the specific requirements
    }

    buffer[numBytes] = '\0'; // Null terminator to make it a string

    // Output the server's response for debugging or information purposes
    //printf("Server Response: %s\n\n", buffer);

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
}






// Function to log into the server over an SSL connection
void login_ssl(SSL *ssl, const char* username, const char* password) {
    char command[BUFFER_SIZE_TLS]; // Buffer to hold the complete command string

    // Format the login command according to the IMAP protocol
    snprintf(command, sizeof(command), "A01 LOGIN %s %s\r\n", username, password); 

    // Send the login command to the server
    send_ssl_command(ssl, command); 

    // Read the response to the login command
    read_ssl_response(ssl); // No need to pass the tag, the function can handle the response checking
}




// Function to select a folder to read from over an SSL connection 
void select_folder_ssl(SSL *ssl, const char *folder_name) {
    char command[BUFFER_SIZE];

    // Use "INBOX" if folder_name is NULL or empty
    if (folder_name == NULL || strlen(folder_name) == 0) {
        folder_name = "INBOX";
    }

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
    snprintf(command, 3*BUFFER_SIZE, "A02 SELECT \"%s\"\r\n", escaped_name);

    // Send the command to the server
    send_ssl_command(ssl, command); 

    // Read the response
    read_ssl_response(ssl);
}




// Function to read FETCH response from an SSL connection
void read_ssl_fetch_response(SSL *ssl, list_t *packet_list, const char *tag) {
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int numBytes = SSL_read(ssl, buffer, BUFFER_SIZE - 1);
        if (numBytes < 0) {
            perror("ERROR reading from SSL connection");
            ERR_print_errors_fp(stderr);
            exit(2);
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
        if (strstr(buffer, tag)) {
            if (strstr(buffer, "OK")) {
                break;  // Exit the loop after storing the last packet
            } else if (strstr(buffer, "NO") || strstr(buffer, "BAD")) {
                printf("Message not found\n");
                free_list(packet_list); // Free the list before exiting
                exit(3);  // Exit with status 3
            }
        }
    }
}


// Function to retrieve an email over an SSL connection
void retrieve_ssl(SSL *ssl, int message_num, list_t *packet_list) {
    // The command is retrieve, we need to fetch the email:
    // tag FETCH messageNum BODY.PEEK[]

    // if messageNum = -1, it means the message sequence number was not specified on the command line 
    // fetch the last added message in the folder 

    char command[BUFFER_SIZE];

    if (message_num == -1) {
        // Fetch the last message (assuming UIDNEXT is available from SELECT)
        snprintf(command, BUFFER_SIZE, "A03 FETCH * BODY.PEEK[]\r\n");
    } else {
        // Fetch a specific message with messageNum
        snprintf(command, BUFFER_SIZE, "A03 FETCH %d BODY.PEEK[]\r\n", message_num);
    }

    // Now we send the FETCH command
    send_ssl_command(ssl, command); 

    // Read the response and store packets in a linked list
    read_ssl_fetch_response(ssl, packet_list, "A03");
}







//////////// OPENSSL Library Initialisation Functions ///////////////////////////

// Function to initialise OpenSSL Libraries
void initialize_openssl() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
}

// Function to create an SSL context using a CA certificate that we stored as "ca.crt"
SSL_CTX* create_ssl_context(const char* ca_cert_file) {
    SSL_CTX* ctx = SSL_CTX_new(SSLv23_client_method());
    if (!ctx) {
        fprintf(stderr, "Error creating SSL context\n");
        ERR_print_errors_fp(stderr);
        exit(2);
    }

    if (!SSL_CTX_load_verify_locations(ctx, ca_cert_file, NULL)) {
        fprintf(stderr, "Error loading root CA certificate\n");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        exit(2);
    }

    return ctx;
}

// Function to create an SSL connection using the context and socket file descriptor
SSL* create_ssl_connection(SSL_CTX* ctx, int sockfd) {
    SSL* ssl = SSL_new(ctx);
    if (!ssl) {
        fprintf(stderr, "Error creating SSL object\n");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    SSL_set_fd(ssl, sockfd);

    if (SSL_connect(ssl) <= 0) {
        fprintf(stderr, "Error performing SSL handshake\n");
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }

    return ssl;
}

// Function to clean up the connection when it is closed. 
void cleanup_ssl(SSL* ssl, SSL_CTX* ctx) {
    if (ssl) {
        SSL_free(ssl);
    }
    if (ctx) {
        SSL_CTX_free(ctx);
    }
    EVP_cleanup();
}
