#ifndef HISTORY_H
#define HISTORY_H

extern const char *histfn;

void init_history(void);
int read_history(const char *);
void print_history(void);
void save_history(void);
void destroy_history(void);
void add_history(const char *);
void history_usage(void);
char *lookup_history(char *s);

#endif
