#ifndef NODE_H
#define NODE_H

typedef enum node_type_e {
    NODE_COMMAND,
    NODE_VAR,
} node_type_e;

typedef enum val_type_e {
    VAL_SINT = 1,
    VAL_UINT,
    VAL_SLLONG,
    VAL_ULLONG,
    VAL_FLOAT,
    VAL_LDOUBLE,
    VAL_CHR,
    VAL_STR,
} val_type_e;

typedef union symval_u {
    long sint;
    unsigned long uint;
    long long sllong;
    unsigned long long ullong;
    double sfloat;
    long double ldouble;
    char chr;
    char *str;
} symval_u;

typedef struct node_s {
    node_type_e type;
    val_type_e val_type;
    symval_u val;
    size_t children;
    struct node_s *first_child;
    struct node_s *next_sibling, *prev_sibling;
} node_s;

node_s *new_node(node_type_e);
void add_child_node(node_s *, node_s *);
void free_node_tree(node_s *);
void set_node_val_str(node_s *, char *);

#endif
