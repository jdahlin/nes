#ifndef __CPU_H__
#define __CPU_H__

#include <stdint.h>

#include "emu.h"

cpu_t* cpu_create(emu_t *emu);
void cpu_map(cpu_t   *cpu,
	     uint16_t dest,
	     const uint8_t *src,
	     uint16_t  size);
void cpu_run(cpu_t *cpu,
	     uint16_t address);
void cpu_dump(cpu_t *cpu);

#endif /* __CPU_H__ */
