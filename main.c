#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


#include "imap_client.h"
#include "utils.h"
#include "tls.h"

int main(int argc, char *argv[]) {

    // Set up signal handler for SIGPIPE
    signal(SIGPIPE, handle_sigpipe);


    // Initialise the fetch mail struct
    fetch_mail_t fetch_mail = {NULL, NULL, NULL, -1, 0, NULL, NULL};

    // Call parse args to read in the command line arguments present
    int parse_arg = parse_args(argc, argv, &fetch_mail);

    if (parse_arg != 1) {
        printf("Failed -- Return Code: %d\n", parse_arg);
        exit(1);
    }

    int sockfd = 0; 

    list_t *packet_list = make_empty_list(); // create an empty list 
    list_t *subject_list = make_empty_list(); // Creates empty list for all the subject lines (Task 2.6 List)
    list_t *header_list = make_empty_list(); // Creates empty list for all headers (Task 2.4 Parse)

    // Now implement logging on and selecting folder
    // log in using: tag LOGIN <username> <password> \r\n
    // using the tag A01


    // If TLS config, we run the tls connect, login_ssl, select_folder_ssl and retrieve_ssl
    if (fetch_mail.isTLS) {
        //printf("*************Is TLS************\n");
        SSL *ssl = NULL;
        sockfd = tls_connect(fetch_mail.server_name, "993", &ssl); // we want to establish tsl 
        login_ssl(ssl, fetch_mail.username, fetch_mail.password);

        select_folder_ssl(ssl, fetch_mail.folder);

        retrieve_ssl(ssl, fetch_mail.messageNum, packet_list);
        
    } else {
        sockfd = create_connection(fetch_mail.server_name, "143");  // we connect the socket in this
        read_response(sockfd, ""); // read and discard initial server greeting message

        // Perform login 
        //printf("\nLOGGING IN\n");
        login(sockfd, fetch_mail.username, fetch_mail.password);

        // Select folder
        // printf("\nSELECTING FOLDER\n");
        // printf("%s", fetch_mail.folder);
        select_folder(sockfd, fetch_mail.folder);

        // retrieve and store response from server into a linked list
        retrieve(sockfd, fetch_mail.messageNum, packet_list);
    }




    // If conditions to run the appropriate command given
    if (strcmp(fetch_mail.command, "retrieve") == 0){
        // To print raw email 
        //printf("\n \n\n-----------------RETRIEVE MESSAGE-----------\n");
        print_list_retrieve(packet_list);
        exit(0);
    }

    if (strcmp(fetch_mail.command, "parse") == 0) {
        parse(sockfd, fetch_mail.messageNum, header_list);
    }

    if (strcmp(fetch_mail.command, "mime") == 0) {
        //printf("\n------------------------MIME----------------------------\n");
        mime(sockfd, packet_list); 
    }

    if (strcmp(fetch_mail.command, "list") == 0) {
        // To fetch email headers and parse them and print them to stdout
        list(sockfd, fetch_mail.messageNum, subject_list);
    }



    free_list(packet_list);
    free_list(subject_list);
    free_list(header_list);


    return 0;
}




