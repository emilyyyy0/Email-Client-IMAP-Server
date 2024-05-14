#ifndef SERVER_RESPONSE_H
#define SERVER_RESPONSE_H

#include <math.h>

// linked list node
typedef struct node node_t;

struct node {
    char *packet;
    node_t *next;
};


// construct a linked list
typedef struct {
    node_t *head;
    node_t *foot;
}list_t;

// create an empty linked list 
list_t *make_empty_list(void);

// function to append node to end of the list. Append to foot
void insert_at_foot(list_t *list, char *packet);

// print packet
void print_packet(node_t *node);

// prints the whole list
void print_list(list_t *list);

// Function to free the process linked list node
void free_node(node_t *node);

// Function to free the entire linked list
void free_list(list_t *list);

// Function that finds length of list.
int len_list(list_t *list);

// Function that checks if list is empty
int is_list_empty(list_t *list);

// Function to print raw email for retrieve 
void print_list_retrieve(list_t *list);

void printUpToIndex(char *string, int index);





#endif