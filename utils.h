#ifndef UTILS_H
#define UTILS_H

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "imap_client.h"

// Function to read in command line arguments
void parse_args(int argc, char *argv[], fetch_mail_t *fetch_mail);

void to_lowercase(char *str);

char *strcasestr(const char *haystack, const char *needle);




#endif 