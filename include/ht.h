#ifndef HT_H
#define HT_H

#include <stdbool.h>

typedef struct ht_s ht_s;

ht_s *ht_create(void);                              // Create a new empty dictionary
void ht_print(ht_s *);                              // Print all dictionary entries
void ht_destroy(ht_s *);                            // Destroy a dictionary
bool ht_insert(ht_s *, const char *, const char *); // Insert a new key-value pair into an existing dictionary
char *ht_search(ht_s *, const char *);              // Return the most recently inserted value associated with a key
void ht_delete(ht_s *, const char *);               // Delete the most recently inserted entry with the given key

#endif
