/**
 * @file history.h
 * @brief Basic shell command history interface
 *
 * Provides core history functionality including initialization, adding
 * commands, saving, and lookup operations.
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 */

#ifndef HISTORY_H
#define HISTORY_H

void init_history(void);
void history_add(const char *);
void history_save(void);
void history_print(void);
char *history_lookup(const char *s);
void history_usage(void);

#endif
