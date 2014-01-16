#ifndef __PPU_H__
#define __PPU_H__

#include <stdbool.h>
#include <stdint.h>

#include "ppu.h"

typedef struct {
  int ticks;
  int scanline;
  // 0x2000: -w PPUCTRL
  // 0x2001: -w PPUMASK
  // 0x2002: r- PPUSTATUS
  // 0x2003: -w OAMADDR
  // 0x2004: rw OAMDATA
  // 0x2005: -w PPUSCROLL
  // 0x2006: -w PPUADDR
  // 0x2007: rw PPUDATA
  uint8_t regs[8];
  // 8Kb of VRAM;
  uint8_t *mem;
  uint16_t pc;
  uint16_t framecount;
} ppu_t;

ppu_t* ppu_create (void);
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
