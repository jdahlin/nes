#include <stdio.h>

#include "emu.h"

int main(int argc, char **argv)
{
    emu_t *emu;

    if (argc < 2) {
       printf("need a filename\n");
       return 1;
    }

    emu = emu_create();
    emu_load(emu, argv[1]);
    emu_run(emu);

    printf("okay\n");
    return 0;
}
