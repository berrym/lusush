#ifndef __PROMPT_H__
#define __PROMPT_H__

typedef enum {
    BLACK = 30,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE
} FG_COLOR;

typedef enum {
    BG_BLACK = 40,
    BG_RED,
    BG_GREEN,
    BG_YELLOW,
    BG_BLUE,
    BG_MAGENTA,
    BG_CYAN,
    BG_WHITE
} BG_COLOR;

typedef enum {
    OFF = 0,
    BOLD = 1,
    UNDERSCORE = 4,
    BLINK = 5,
    REVERSE = 7,
    CONCEALED = 8
} COLOR_ATTRIB;

extern void set_prompt_fg(FG_COLOR);
extern void set_prompt_bg(BG_COLOR);
extern void set_prompt_attr(COLOR_ATTRIB);
extern void set_prompt(int, char **);
extern void build_prompt(void);

#endif
