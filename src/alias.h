#ifndef __ALIAS_H__
#define __ALIAS_H__

extern int set_alias(char *, char *);
extern void unset_alias(char *);
extern char *expand_alias(char *);
extern void set_max_alias(size_t);
extern void print_alias(void);

#endif
