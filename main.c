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
    // printf("\n\n---------------------------NEW------------------------\n\n");
    // printf("Username: %s\n", fetch_mail.username);
    // printf("Password: %s\n", fetch_mail.password);
    // printf("Folder: %s\n", fetch_mail.folder); //? fetch_mail.folder : "not specified");
    // printf("MessageNum: %d\n", fetch_mail.messageNum);
    // printf("isTSL: %d\n", fetch_mail.isTSL);
    // printf("Command: %s\n", fetch_mail.command);
    // printf("Server Name: %s\n\n", fetch_mail.server_name);

    int sockfd = 0; 


    // If TSL config 
    if (fetch_mail.isTSL) {
        printf("*************Is TSL************\n");
        sockfd = create_connection(fetch_mail.server_name, "993");
    } else {
        sockfd = create_connection(fetch_mail.server_name, "143");  // we connect the socket in this
    }

    //printf("sockfd = %d \n", sockfd);

    // Now we must implement logging on 
    // log in using: tag LOGIN <username> <password> \r\n
    // if log on is successful, we will receive a string 

    // we will use the tag A01

    // IMAP login 
    // char command[BUFFER_SIZE];
    // char tag[10];
    // snprintf(tag, sizeof(tag), "");
    // this line constructs the IMAP login command using snprintf
    // A01 : tag
    // LOGIN: IMAP command keyword, indicates login operation
    
    // Command string is sent to IMAP server over the socket connection
    //send_command(sockfd, ""); 

    // Read the server's response to the initial greeting to server. 
    //printf("INITIAL SERVER GREETING: \n");
    read_response(sockfd, ""); // read and discard initial server greeting message

    // Perform login 
    //printf("\nLOGGING IN\n");
    login(sockfd, fetch_mail.username, fetch_mail.password);

    // Select folder
    //printf("\nSELECTING FOLDER\n");
    select_folder(sockfd, fetch_mail.folder);

    list_t *packet_list = make_empty_list(); // create an empty list 
    retrieve(sockfd, fetch_mail.messageNum, packet_list);



    // If conditions to run the appropriate command given
    if (strcmp(fetch_mail.command, "retrieve") == 0){
        // To retrieve and print email
        //retrieve(sockfd, fetch_mail.messageNum, packet_list);
        //printf("\n \n\n-----------------RETRIEVING MESSAGE-----------\n");
        print_list_retrieve(packet_list); // Or iterate through the list using head and next pointers
    }

    if (strcmp(fetch_mail.command, "mime") == 0) {
        //printf("\n------------------------MIME----------------------------\n");
        mime(sockfd, packet_list); 
    }

    free_list(packet_list);


    return 0;
}

// REMEMBER TO FREE THE LIST 



