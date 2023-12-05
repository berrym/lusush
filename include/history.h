#ifndef HISTORY_H
#define HISTORY_H

void init_history(void);
void history_add(const char *);
void history_save(void);
void history_print(void);
char *history_lookup(const char *s);
void history_usage(void);

#endif
