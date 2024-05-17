#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "imap_client.h"


// Signal handler for SIGPIPE
void handle_sigpipe(int sig) {
    fprintf(stderr, "Error: Received SIGPIPE signal. Connection closed unexpectedly.\n");
    exit(2); // Exit with return code 2 for unexpected disconnection
}



// Function to print usage instructions and exit
void print_usage() {
    fprintf(stderr, "Usage: ./fetchmail -n <number> -u <username> -p <password> -f <folder> <command> <server>\n");
    exit(1);
}

// Function to parse the -n value
int parse_n_value(const char *n_str) {
    char *endptr;
    errno = 0;
    long n = strtol(n_str, &endptr, 10);

    if (errno == ERANGE || n < 0 || n > INT_MAX_UTIL || endptr == n_str || *endptr != '\0') {
        fprintf(stderr, "Invalid -n value: %s\n", n_str);
        print_usage();
    }

    return (int)n;
}

void check_for_injection(const char *input) {
    if (strchr(input, '\n') != NULL || strchr(input, '\r') != NULL) {
        fprintf(stderr, "Error: Input contains illegal characters.\n");
        print_usage();
    }
}



// A function to read in the command line arguments given and store them in the fetchmail struct
int parse_args(int argc, char *argv[], fetch_mail_t *fetch_mail) {

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-u") == 0 && i + 1 < argc) {
            fetch_mail->username = argv[++i];
            check_for_injection(fetch_mail->username);
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            fetch_mail->password = argv[++i];
            check_for_injection(fetch_mail->password);
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            fetch_mail->folder = argv[++i];
            if (strlen(fetch_mail->folder) == 0) {
                fprintf(stderr, "Error: Folder name cannot be an empty string.\n");
                print_usage();
            }
            check_for_injection(fetch_mail->folder);
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            fetch_mail->messageNum = parse_n_value(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0) {
            fetch_mail->isTLS = 1;
        } else if (fetch_mail->command == NULL) {
            fetch_mail->command = argv[i];
            check_for_injection(fetch_mail->command);
        } else if (fetch_mail->server_name == NULL) {
            fetch_mail->server_name = argv[i];
            check_for_injection(fetch_mail->server_name);
        } else {
            fprintf(stderr, "No recognisable arguments found: %s\n", argv[i]);
            print_usage();
        }
    }

    if (!fetch_mail->username || !fetch_mail->password || !fetch_mail->command || !fetch_mail->server_name) {
        fprintf(stderr, "Error: Missing one or more core arguments.\n");
        print_usage();
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