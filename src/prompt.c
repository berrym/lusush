#include "../include/symtable.h"
#include <sys/types.h>
#include <unistd.h>

void build_prompt(void) {
    symtable_entry_s *prompt_entry = add_to_symtable("PS1");

    if (getuid() > 0)
        symtable_entry_setval(prompt_entry, "% ");
    else
        symtable_entry_setval(prompt_entry, "# ");
}
