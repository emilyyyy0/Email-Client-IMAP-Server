#ifndef IMAP_CLIENT_H
#define IMAP_CLIENT_H // this is changed in my own code

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#include "server_response.h"

#define BUFFER_SIZE 2048


// Struct to store the command line arguments
typedef struct fetch_mail {
        char *username; 
        char *password; 
        char *folder;
        int messageNum;
        int isTSL;
        char *command;
        char *server_name;
} fetch_mail_t;

// // Function to read in command line arguments
// void parse_args(int argc, char *argv[], fetch_mail_t *fetch_mail);

int create_connection(const char *hostname, const char *port);

void send_command(int sockfd, const char *cmd);

void error(const char *msg, int num);

void read_response(int sockfd, const char* tag); 

void login(int sockfd, const char* username, const char* password);

void select_folder(int sockfd, const char *folder_name);

void retrieve(int sockfd, int message_num, list_t *packet_list);

void mime(int sockfd, list_t *packet_list);

char *concatenate_packets(list_t *packet_list);

char *find_mime_boundary(const char *content);

void parse_mime_parts(const char *email_content, const char *boundary);

void decode_quoted_printable(const char *input, char *output);

void print_body_up_to_boundary(const char *body, const char *boundary);


#endif