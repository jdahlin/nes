/* Global emulator structures */

#include <stdlib.h>

#include "ines.h"
#include "emu.h"
#include "cpu.h"
#include "ppu.h"

emu_t*
emu_create(void)
{
  emu_t * emu;

  emu = (emu_t*)calloc(sizeof(emu_t), 1);
  emu->cpu = cpu_create(emu);
  emu->ppu = ppu_create(emu);

  return emu;
}

void
emu_load(emu_t *emu, const char *filename)
{
    ines_t *nes;

    nes = ines_load(filename);
    cpu_map(emu->cpu, 0x8000, nes->prg, ines_prg_size(nes) * 1024);
    ppu_map(emu->ppu, 0x0000, nes->chr, ines_chr_size(nes) * 1024);
}

void
emu_run(emu_t *emu)
{
    cpu_run(emu->cpu, 0x8000);
}
