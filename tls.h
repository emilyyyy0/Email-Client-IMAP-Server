#ifndef TLS_H
#define TLS_H

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "server_response.h"

#define BUFFER_SIZE_TLS 4906



// Function to connect using TLS
int tls_connect(const char *hostname, const char *port, SSL **ssl_out);

// Function to clean up SSL resources
void cleanup_ssl(SSL* ssl, SSL_CTX* ctx);

// Function to initialize OpenSSL
void initialize_openssl();

// Function to create and configure an SSL context
SSL_CTX* create_ssl_context(const char* ca_cert_file);

// Function to create an SSL connection
SSL* create_ssl_connection(SSL_CTX* ctx, int sockfd);

void print_server_response(SSL *ssl);

void read_initial_server_response(SSL *ssl);

void send_ssl_command(SSL *ssl, const char *command);

void login_ssl(SSL *ssl, const char* username, const char* password);

void read_ssl_response(SSL *ssl);

void select_folder_ssl(SSL *ssl, const char *folder_name);


void retrieve_ssl(SSL *ssl, int message_num, list_t *packet_list);

void read_ssl_fetch_response(SSL *ssl, list_t *packet_list, const char *tag);

#endif // TLS_H
