#include <stdbool.h>

#ifndef DICT_H
#define DICT_H

typedef struct dict_s dict_s;

dict_s *dict_create(void);                              // Create a new empty dictionary
void print_dict(dict_s *);                              // Print all dictionary entries
void dict_destroy(dict_s *);                            // Destroy a dictionary
bool dict_insert(dict_s *, const char *, const char *); // Insert a new key-value pair into an existing dictionary
char *dict_search(dict_s *, const char *);              // Return the most recently inserted value associated with a key
void dict_delete(dict_s *, const char *);               // Delete the most recently inserted entry with the given key

#endif
