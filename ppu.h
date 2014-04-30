#ifndef __PPU_H__
#define __PPU_H__

#include <stdbool.h>
#include <stdint.h>

#include "types.h"

ppu_t* ppu_create(emu_t *emu);
void ppu_map(ppu_t   *ppu,
	     uint16_t dest,
	     const uint8_t *src,
	     uint16_t  size);
void ppu_write(ppu_t   *ppu,
	       uint16_t addr,
	       uint8_t  value);
uint8_t ppu_read(ppu_t   *ppu,
		 uint16_t addr);
bool ppu_nmi_is_enabled(ppu_t *ppu);
void ppu_nmi_disable(ppu_t *ppu);
void ppu_run(ppu_t *ppu,
	     int cycles);


#endif /* __PPU_H__ */
