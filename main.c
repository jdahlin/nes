#include <stdio.h>

#include "ines.h"
#include "cpu.h"
#include "ppu.h"

int main(int argc, char **argv)
{
    ines_t *nes;
    cpu_t *cpu;

    if (argc < 2) {
       printf("need a filename\n");
       return 1;
    }

    nes = ines_load(argv[1]);

    cpu = cpu_create();
    cpu_map(cpu, 0x8000, nes->prg, ines_prg_size(nes) * 1024);
    ppu_map(cpu->ppu, 0x0000, nes->chr, ines_chr_size(nes) * 1024);
    cpu_run(cpu, 0x8000);


    printf("okay\n");
    return 0;
}
