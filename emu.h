#ifndef __EMU_H__
#define __EMU_H__

#include "types.h"

emu_t* emu_create(void);
void emu_load(emu_t *emu, const char *filename);
void emu_run(emu_t *emu);

#endif /* __EMU_H__ */
