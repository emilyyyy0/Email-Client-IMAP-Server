#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#include "server_response.h"

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


// Insert a node at the foot of the linked list 
// Inserts a new node with value "process" to the end of "list" 
void insert_at_foot(list_t *list, char *packet) {
    // Ensure that list and packet are not NULL
    assert(list != NULL && packet != NULL);

    // Creates a new node with packet inside of it
    node_t *new = (node_t*)malloc(sizeof(*new));
    if (new == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for the packet data
    new->packet = (char*)malloc(strlen(packet) + 1);
    if (new->packet == NULL) {
        perror("Memory allocation failed");
        free(new); // Free the allocated node memory
        exit(EXIT_FAILURE);
    }

    // Stores response in packet->response
    // Copy the data from packet into the newly allocated memory with bounds checking
    strncpy(new->packet, packet, strlen(packet) + 1);

    new->next = NULL; // this points to NULL as it is the end of the list
 

    if (list->head == NULL) {
        // First insert into the list 
        // new becomes the only node in the list
        list->head = list->foot = new;
    } else {
        list->foot->next = new; // old tail connected to "new"
        list->foot = new; // new foot of the list 
    }
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
