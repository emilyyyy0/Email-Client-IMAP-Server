#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imap_client.h"

int main(int argc, char *argv[]) {
    // Initialise the fetch mail struct
    fetch_mail_t fetch_mail = {NULL, NULL, NULL, -1, 0, NULL, NULL};

    // Call parse args to read in the command line arguments present
    parse_args(argc, argv, &fetch_mail);

    return 0;
}


// A function to read in the command line arguments given and store them in the fetchmail struct
void parse_args(int argc, char *argv[], fetch_mail_t *fetch_mail) {

    // Reads in all the command arguments given and stores them in the appropriate variable
    for (int i = 1; i < argc; i++) {
      
        if (strcmp(argv[i], "-u") == 0 && i + 1 < argc) {
            fetch_mail->username = argv[++i];
        } 
        else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            fetch_mail->password = argv[++i];
        } 
        else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            fetch_mail->folder = argv[++i];
        } 
        else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            fetch_mail->messageNum = atoi(argv[++i]);
        } 
        else if (strcmp(argv[i], "-t") == 0) {
            fetch_mail->tag = 1;
        } 
        else if (fetch_mail->command == NULL) {
            fetch_mail->command = argv[i];
        } 
        else if (fetch_mail->server_name == NULL) {
            fetch_mail->server_name = argv[i];
        } 
        
        // Error condition if the argument being read isn't one of the recognised command line arguments
        else {
            printf("No recognisable arguments found\n");
        }
    }

    // Checks if all four core arguments have been stored
    if (!fetch_mail->username || !fetch_mail->password || !fetch_mail->command || !fetch_mail->server_name) {
        printf("At least one of the four core arguments cannot be found\n");
    }

    // Print stored command arguments
    printf("Username: %s\n", fetch_mail->username);
    printf("Password: %s\n", fetch_mail->password);
    printf("Folder: %s\n", fetch_mail->folder ? fetch_mail->folder : "not specified");
    printf("MessageNum: %d\n", fetch_mail->messageNum);
    printf("Tag: %d\n", fetch_mail->tag);
    printf("Command: %s\n", fetch_mail->command);
    printf("Server Name: %s\n", fetch_mail->server_name);
}

