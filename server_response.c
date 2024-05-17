#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#include "server_response.h"

#include "imap_client.h"

/***
 * This is a file to store the server response into a linked list since the server may respond in packets if too large. 
*/

// create an empty list 
list_t *make_empty_list(void) {
    list_t *list;
    list = (list_t*)malloc(sizeof(*list));
    assert(list != NULL);
    list->head = list->foot = NULL;
    //printf("list successfully created\n");
    return list;

}

// Function to check if the linked list is empty
int is_list_empty(list_t *list) {
    if (list == NULL || list->head == NULL) {
        return 1; // The list is empty 
    }
    return 0; // The list is not empty
}



// A function to print the packet
void print_packet(node_t *node) {
    printf("%s\n", node->packet);
}

// Function to traverse and print all the packets = server response
void print_list(list_t *list) {
    node_t *current = list->head; // Start from the head of the list

    while (current != NULL) { 
        print_packet(current); 
        current = current->next; 
    }
}

// Function to free the linked list node
void free_node(node_t *node) {
    if (node != NULL) {
        free(node->packet);
        free(node->type);
        free(node);
    }
}

// Function to free the entire linked list
void free_list(list_t *list) {
    if (list != NULL) {
        // Traverse the list and free each node
        node_t *current = list->head;
        node_t *next;

        while (current != NULL) {
            next = current->next;
            free_node(current);  
            current = next;
        }
    
        free(list);
    }
}

// A function to get the length of the process queue
int len_list(list_t *list) {
    int length = 0;

    node_t *current = list->head; // Start from the head of the list

    while (current != NULL) { 
        current = current->next; 
        length++;
    }

    return length;

}



void printUpToIndex(char *string, int index) {
    if (index < 0 || index >= strlen(string)) {
        printf("Invalid index\n");
        return;
    }

    char temp = string[index]; // Save the character at the index
    string[index] = '\0';      // Null-terminate the string at the index
    printf("%s\n", string);    // Print the string
    string[index] = temp;      // Restore the original character
}


void insert_at_foot(list_t *list, const char *packet, const char *type) {
    assert(list != NULL && packet != NULL);

    // Memory allocation of the linked list node
    node_t *new_node = (node_t *)malloc(sizeof(*new_node));
    if (new_node == NULL) {
        perror("Failed to allocate memory for new node");
        exit(EXIT_FAILURE);
    }

    // Memory allocation for packet
    new_node->packet = (char *)malloc(strlen(packet) + 1);
    if (new_node->packet == NULL) {
        perror("Failed to allocate memory for packet");
        free(new_node); // Clean up the node itself
        exit(EXIT_FAILURE);
    }
    strcpy(new_node->packet, packet); // Copy data into packet

    // Memory allocation for node type (if applicable ---> for header_list only)
    if (type != NULL) {
        new_node->type = (char *)malloc(strlen(type) + 1);
        if (new_node->type ==
            NULL) { // Always good to check the result of malloc
            perror("Failed to allocate memory for type");
            free(new_node->packet); // Clean up previously allocated memory
            free(new_node);         // Clean up the node itself
            exit(EXIT_FAILURE);
        }
        strcpy(new_node->type, type); // Copy data into type
    } else {
        new_node->type = NULL;
    }

    // Initialization of node and list pointers
    new_node->next = NULL;
    if (list->head == NULL) {
        list->head = list->foot = new_node;
    } else {
        list->foot->next = new_node;
        list->foot = new_node;
    }
}



// Bubble sort function to sort subject_list by type (message sequence number)
void sort_subject_list(list_t *list) {
    // Check if the list is empty
    if (list->head == NULL) {
        printf("No head found\n");
        exit(3); // Exit if the list is empty
    }

    int swapped;
    node_t *ptr1;
    node_t *lptr = NULL; // Last pointer to mark the end of the unsorted part

    // Perform bubble sort
    do {
        swapped = 0;       // Reset swapped flag
        ptr1 = list->head; // Start from the head of the list

        // Traverse the list up to the last sorted element
        while (ptr1->next != lptr) {
            // Compare the sequence numbers of the current node and the next
            // node
            if (atoi(ptr1->type) > atoi(ptr1->next->type)) {
                // Swap the sequence numbers and subject data
                char *temp_type = ptr1->type;
                char *temp_packet = ptr1->packet;

                ptr1->type = ptr1->next->type;
                ptr1->packet = ptr1->next->packet;

                ptr1->next->type = temp_type;
                ptr1->next->packet = temp_packet;

                swapped = 1; // Set swapped flag
            }
            ptr1 = ptr1->next; // Move to the next node
        }
        lptr = ptr1; // Update the last sorted element
    } while (swapped); // Repeat until no swaps are made
}

// Function to print the subject list in desired format
void print_subject_list(list_t *subject_list) {
    node_t *current = subject_list->head; // Start from the head of the list
    // Check print
    while (current != NULL) {
        printf("%s: %s\n", current->type, current->packet);
        current = current->next;
    }
}




// Function to print the string with NUL byte handling
void print_string_with_nul_handling(const char *str) {
    while (*str) {
        if (*str == '\0') {
            putchar(NUL_PLACEHOLDER);
        } else {
            putchar(*str);
        }
        str++;
    }
    putchar('\n');
}

// Function to extract and print the desired portion of the email
void print_list_retrieve(list_t *list) {
    char *res = concatenate_packets(list);

    // Find the start of the email content after the initial FETCH line
    char *email_start = strstr(res, "\n");
    if (email_start) {
        email_start++;  // Move past the newline character

        // Find the position of the "A03 OK" line
        char *end_marker = strstr(email_start, "A03 OK");
        if (end_marker) {
            // Create a copy of the email content up to the end marker
            size_t content_length = end_marker - email_start;
            char *email_content = (char *)malloc(content_length + 1);
            if (email_content == NULL) {
                perror("Failed to allocate memory");
                free(res);
                exit(EXIT_FAILURE);
            }
            strncpy(email_content, email_start, content_length);
            email_content[content_length] = '\0';

            // Remove trailing ")" and one trailing newline
            char *trim_pos = email_content + content_length - 1;
            int removed_trailing_parenthesis = 0;
            while (trim_pos >= email_content) {
                if (*trim_pos == ')') {
                    *trim_pos = '\0';
                    removed_trailing_parenthesis = 1;
                    trim_pos--;
                    break;
                } else if (*trim_pos == '\n' || *trim_pos == '\r') {
                    *trim_pos = '\0';
                    trim_pos--;
                } else {
                    break;
                }
            }

            // Remove one more trailing newline if the flag is set
            if (removed_trailing_parenthesis) {
                if (*trim_pos == '\n' || *trim_pos == '\r') {
                    *trim_pos = '\0';
                }
            }

            // Print the trimmed email content with NUL byte handling
            print_string_with_nul_handling(email_content);
            free(email_content);
        } else {
            // Print the entire remaining content if "A03 OK" is not found
            print_string_with_nul_handling(email_start);
        }
    }

    free(res);  // Free the allocated memory for the concatenated string
}



// Function to traverse and print all the packets = server response
// void print_list_retrieve(list_t *list) {
//     node_t *current = list->head; // Start from the head of the list

//     // char *res = concatenate_packets(list); 
//     // printf("%s res\n", res);

//     int firstLine = 0; // flag for the first line which has FETCH

//     // iterate through the packets linked list 
//     while (current != NULL) { 
//         int i = 0;

//         char *str = current->packet; 

//         // while loop that finds the first '\n' 
//         while(!firstLine) {
//             if (str[i] == '\n') {
//                 // firstLine = 1; 
//                 // printf("%s", str + i + 1);
//                 break;
//             }

//             i++; 
//         }


//         // For the last packet 
//         if (strstr(current->packet, "A03 OK")) {
//             int length = strlen(str); // get length of string
//             int newLineCount = 0; 

//             for (int i = length - 1; i >= 0; i--) {
//                 if (str[i] == '\n') {
//                     newLineCount++;
//                     if (newLineCount == 2) {
//                         // printf("%s", str + i + 1);
//                         printUpToIndex(str, i - 3);
//                         break;
//                     }
//                 }
//             }
//             break;
//         }

//         if (firstLine) { // means we have printed skipped the firstline already 
//             printf("%s", str);
//         } else if (!firstLine) {
//             printf("%s", str + i + 1);
//             firstLine = 1;
//         }


//         current = current->next; 
//     }
// }