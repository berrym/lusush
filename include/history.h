#ifndef HISTORY_H
#define HISTORY_H

extern char *histfn;

#if !defined(USING_READLINE_HISTORY)

struct histable {
    size_t len;
    struct histentry *head;
    struct histentry *tail;
};

struct histentry {
    char *cmd;
    struct histentry *next;
};

extern struct histable *hist_list;

#else

extern char *hist_list;

#endif

void init_history(void);
int read_history(const char *);
void print_history(void);
void save_history(void);
void free_history_list(void);
void add_to_history(const char *);
const char *hist_file_name();

#endif
