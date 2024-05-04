#ifndef IMAP_CLIENT_H
#define IMAP_CLIENT_H // this is changed in my own code

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

int create_connection();

#endif