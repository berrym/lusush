#ifndef NODE_H
#define NODE_H

enum node_type {
    NODE_COMMAND,
    NODE_VAR,
};

enum val_type {
    VAL_SINT = 1,
    VAL_UINT,
    VAL_SLLONG,
    VAL_ULLONG,
    VAL_FLOAT,
    VAL_LDOUBLE,
    VAL_CHR,
    VAL_STR,
};

union symval {
    long sint;
    unsigned long uint;
    long long sllong;
    unsigned long long ullong;
    double sfloat;
    long double ldouble;
    char chr;
    char *str;
};

struct node {
    enum node_type type;
    enum val_type val_type;
    union symval val;
    size_t children;
    struct node *first_child;
    struct node *next_sibling, *prev_sibling;
};

struct node *new_node(enum node_type);
void add_child_node(struct node *, struct node *);
void free_node_tree(struct node *);
void set_node_val_str(struct node *, char *);

#endif
