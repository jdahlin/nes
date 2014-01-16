#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ppu.h"

#define TICKS_PER_SCANLINE 341
#define SCANLINE_START_NMI 241 // NTSC
#define SCANLINE_END_FRAME 262 // NTSC

ppu_t*
ppu_create(void)
{
    ppu_t *ppu;
    ppu = calloc(sizeof(ppu_t), 1);
    ppu->mem = calloc(sizeof(uint8_t), 8192);
    return ppu;
}

void
ppu_map(ppu_t         *ppu,
        uint16_t       dest,
        const uint8_t *src,
        uint16_t       size)
{
    memcpy(&ppu->mem[dest], src, size);
}

void
ppu_write(ppu_t   *ppu,
	  uint16_t addr,
	  uint8_t  value)
{
  uint8_t regno = addr & ~0x2ff0;
  ppu->regs[regno] = value;
  switch(regno) {
  case 0x0: { // PPUCTRL
    uint16_t base;
    //printf("PPU Control register #1: $%02X\n", value);
    switch(value & ~0xfd) {
    case 0:
      base = 0x2000;
      break;
    case 1:
      base = 0x2400;
      break;
    case 2:
      base = 0x2800;
      break;
    case 3:
      base = 0x2C00;
      break;
    }
#if 0
    printf("  - base nametable: $%04X\n", base);
    printf("  - increment: %d\n",
	   value >> 2 & 1 ? 32 : 1);
    printf("  - bg pattern table: $%04X\n",
	   value >> 4 & 1 ? 0x1000 : 0);
    printf("  - NMI at start of vblank: %s\n",
	   value >> 7 & 1 ? "yes" : "no");
#endif
    break;
  }
  case 0x1: // PPUMASK
#if 0
    printf("PPU Control register #2: $%02X\n", value);
    printf("  - Display: %s\n",
	   value & 1 ? "mono" : "color");
    printf("  - Background clipping: %s\n",
	   value >> 1 & 1 ? "no clipping" : "not in leftmost 8");
    printf("  - Sprite clipping: %s\n",
	   value >> 2 & 1 ? "no clipping" : "not in leftmost 8");
    printf("  - Background visibility: %s\n",
	   value >> 3 & 1 ? "Display" : "hidden");
    printf("  - Sprite visibility: %s\n",
	   value >> 4 & 1 ? "Display" : "hidden");
    printf("  - Background color: %d (0=black, 1=red, 2=blue, 4=green)\n", 
	   value >> 5);
#endif
    break;
  case 0x3: // OAMADDR
    printf("FIXME: Implement OAMADDR($%02X)\n", value);
    break;
  case 0x4: // OAMDATA
    printf("FIXME: Implement OAMDATA($%02X)\n", value);
    break;
  case 0x5: // PPUSCROLL
    printf("FIXME: Implement PPUSCROLL($%02X)\n", value);
    break;
  case 0x6: // PPUADDR
    ppu->pc <<= 8;
    ppu->pc |= value;
    //printf("ppuaddr: %04X = %02X\n", ppu->pc, value);
    break;
  case 0x7: // PPUDATA
    //printf("ppudata[%04X] = $%02X\n", ppu->pc, value);
    if (ppu->regs[0] >> 2 & 1)
      ppu->pc += 32;
    else
      ppu->pc += 1;
    break;
  default:
    break;
  }
}

uint8_t
ppu_read(ppu_t   *ppu,
	 uint16_t addr)
{
  uint8_t res;
  uint8_t regno = addr & ~0x2ff0;
  res = ppu->regs[regno];
#if 0
  printf("ppu[%x]: ticks=%d scaline=%d -> $%02X\n", 0x2000 + regno, ppu->ticks, 
  	 ppu->scanline, res);
#endif
  return res;
}

void
ppu_draw(ppu_t *ppu)
{
  printf("PPU: DRAW(%d)\n", ppu->framecount++);
}

void
ppu_scanline(ppu_t *ppu)
{
  printf("PPU: scanline: %d\n", ppu->scanline);
  ppu->scanline++;
  if (ppu->scanline == 240) {
    ppu_draw(ppu);
  } else if (ppu->scanline == SCANLINE_START_NMI) {
    ppu->regs[2] |= 0x80;
  } else if (ppu->scanline == SCANLINE_END_FRAME - 1) {
    ppu->scanline = -1;
  }
}

void
ppu_cycle(ppu_t *ppu)
{
  ppu->ticks++;
  if (ppu->ticks == TICKS_PER_SCANLINE) {
    ppu->ticks = 0;
    ppu_scanline(ppu);
  } else if (ppu->scanline == -1 && ppu->ticks == 1) {
    ppu->regs[2] = 0;
  }
}

bool
ppu_nmi_is_enabled(ppu_t *ppu)
{
  return ppu->regs[0] >> 7 & 1;
}

void
ppu_nmi_disable(ppu_t *ppu)
{
  ppu->regs[0] &= ~(1 << 7);
}

void
ppu_run(ppu_t *ppu,
	int cycles)
{
  for (int i = 0; i < cycles; i++) {
    ppu_cycle(ppu);
  }
}
