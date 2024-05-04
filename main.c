#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imap_client.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    // Initialise the fetch mail struct
    fetch_mail_t fetch_mail = {NULL, NULL, NULL, -1, 0, NULL, NULL};

    // Call parse args to read in the command line arguments present
    parse_args(argc, argv, &fetch_mail);

    // Print stored command arguments
    printf("Username: %s\n", fetch_mail.username);
    printf("Password: %s\n", fetch_mail.password);
    printf("Folder: %s\n", fetch_mail.folder ? fetch_mail.folder : "not specified");
    printf("MessageNum: %d\n", fetch_mail.messageNum);
    printf("Tag: %d\n", fetch_mail.tag);
    printf("Command: %s\n", fetch_mail.command);
    printf("Server Name: %s\n", fetch_mail.server_name);

    int check = create_connection(); 

    return 0;
}


