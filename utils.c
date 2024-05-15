#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "imap_client.h"


void print_usage() {
    fprintf(stderr, "Usage: ./fetchmail -n <number> -u <username> -p <password> -f <folder> <command> <server>\n");
    exit(1); // Exit with error code 1: Command-line parsing error, argument validation failure
}

int parse_n_value(const char *n_str) {
    char *endptr;
    errno = 0;
    long n = strtol(n_str, &endptr, 10);

    // Check for various possible errors
    if (errno == ERANGE || n < 0 || n > INT_MAX || endptr == n_str || *endptr != '\0') {
        fprintf(stderr, "Invalid -n value: %s\n", n_str);
        print_usage(); // This will exit with error code 1
    }

    return (int)n;
}


// A function to read in the command line arguments given and store them in the fetchmail struct
int parse_args(int argc, char *argv[], fetch_mail_t *fetch_mail) {

    // Reads in all the command arguments given and stores them in the appropriate variable
    for (int i = 1; i < argc; i++) {
      
        // if (strcmp(argv[i], "-u") == 0 && i + 1 < argc) {
        //     fetch_mail->username = argv[++i];
        // } 
        // else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
        //     fetch_mail->password = argv[++i];
        // } 
        // else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
        //     fetch_mail->folder = argv[++i];
        // } 
        // else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
        //     fetch_mail->messageNum = atoi(argv[++i]);
        // } 
        // else if (strcmp(argv[i], "-t") == 0) {
        //     fetch_mail->isTSL = 1;
        // } 
        // else if (fetch_mail->command == NULL) {
        //     fetch_mail->command = argv[i];
        // } 
        // else if (fetch_mail->server_name == NULL) {
        //     fetch_mail->server_name = argv[i];
        // } 
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
            
            fetch_mail->messageNum = parse_n_value(argv[++i]);
        } 
        else if (strcmp(argv[i], "-t") == 0) {
            fetch_mail->isTSL = 1;
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
        //printf("At least one of the four core arguments cannot be found\n");
        return 0;

    }

    return 1;

}

// Function to convert a string to lowercase
void to_lowercase(char *str) {
    for (; *str; ++str) *str = tolower(*str);
}

// Case-insensitive strstr function
char *strcasestr(const char *haystack, const char *needle) {
    if (!*needle) return (char *) haystack;
    for (; *haystack; ++haystack) {
        if (tolower((unsigned char) *haystack) == tolower((unsigned char) *needle)) {
            const char *h, *n;
            for (h = haystack, n = needle; *h && *n; ++h, ++n) {
                if (tolower((unsigned char) *h) != tolower((unsigned char) *n)) break;
            }
            if (!*n) return (char *) haystack;
        }
    }
    return NULL;
}