#include <unistd.h>
#include <sys/types.h>
#include "symtable.h"

void build_prompt(void)
{
    struct symtable_entry *prompt_entry = add_to_symtable("PS1");

    if (getuid() > 0)
        symtable_entry_setval(prompt_entry, "% ");
    else
        symtable_entry_setval(prompt_entry, "# ");
}
