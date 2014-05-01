/* PPU Emulation of a Ricoh RP2C02 (NTSC)
 * Operates at 5.37Mhz
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "ppu.h"

#define TICKS_PER_SCANLINE 341
#define SCANLINE_START_NMI 241 // NTSC
#define SCANLINE_END_FRAME 262 // NTSC

#define WIDTH 256
#define HEIGHT 240

static uint32_t palette[] = {
  0x666666, 0x002a88, 0x1412a7, 0x3b00a4,
  0x5c007e, 0x6e0040, 0x6c0600, 0x561d00,
  0x333500, 0x0b4800, 0x005200, 0x004f08,
  0x00404d, 0x000000, 0x000000, 0x000000,
  0xadadad, 0x155fd9, 0x4240ff, 0x7527fe,
  0xa01acc, 0xb71e7b, 0xb53120, 0x994e00,
  0x6b6d00, 0x388700, 0x0c9300, 0x008f32,
  0x007c8d, 0x000000, 0x000000, 0x000000,
  0xfffeff, 0x64b0ff, 0x9290ff, 0xc676ff,
  0xf36aff, 0xfe6ecc, 0xfe8170, 0xea9e22,
  0xbcbe00, 0x88d800, 0x5ce430, 0x45e082,
  0x48cdde, 0x4f4f4f, 0x000000, 0x000000,
  0xfffeff, 0xc0dfff, 0xd3d2ff, 0xe8c8ff,
  0xfbc2ff, 0xfec4ea, 0xfeccc5, 0xf7d8a5,
  0xe4e594, 0xcfef96, 0xbdf4ab, 0xb3f3cc,
  0xb5ebf2, 0xb8b8b8, 0x000000, 0x000000,
};

static SDL_Window *win;
static SDL_Renderer *renderer;

ppu_t*
ppu_create(emu_t *emu)
{
    ppu_t *ppu;
    ppu = calloc(sizeof(ppu_t), 1);
    ppu->mem = calloc(sizeof(uint8_t), 0x4000);
    ppu->emu = emu;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
      printf("Unable to initialize SDL: %s\n", SDL_GetError());
      return NULL;
    }
    atexit(SDL_Quit);

    win = SDL_CreateWindow("nes",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           WIDTH, HEIGHT,
                           SDL_WINDOW_SHOWN);
    if (win == NULL) {
      printf("Unable to create SDL window: %s\n", SDL_GetError());
      return NULL;
    }

    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (renderer == NULL) {
      printf("Unable to create SDL renderer: %s\n", SDL_GetError());
      return NULL;
    }
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

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

// CPU $2006, PPUADDR, write x 2
static inline void
ppu_prepare_write_data(ppu_t *ppu, uint8_t value)
{
  //printf("ppuaddr: %04X = %02X\n", ppu->pc, value);
  ppu->pc <<= 8;
  ppu->pc |= value;
}

// CPU $2007, PPUDATA, write
static inline void
ppu_write_data(ppu_t *ppu,
               uint8_t value)
{
  //printf("ppudata[%04X] = $%02X\n", ppu->pc & 0x3fff, value);

  /* Valid addresses are $0000-$3FFF; higher addresses will be mirrored down. */
  ppu->mem[ppu->pc & 0x3fff] = value;

  if (ppu->regs[0] >> 2 & 1)
    ppu->pc += 32;
  else
    ppu->pc += 1;
}

/* addresses are in CPU address space (0x2000..0x3fff) */
void
ppu_write(ppu_t   *ppu,
	  uint16_t addr,
	  uint8_t  value)
{
  uint8_t regno = addr & ~0x2ff0; // There are only 8 registers, so mask out
  ppu->regs[regno] = value;
  switch(regno) {
  case 0x0: { // CPU $2000, PPUCTRL, write
#if 0
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
#endif
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
  case 0x1: // CPU $2001, PPUMASK, write
#if 1
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
  case 0x6:
    ppu_prepare_write_data(ppu, value);
    break;
  case 0x7:
    ppu_write_data(ppu, value);
    break;
  default:
    assert(0);
    break;
  }
}

/* Read CPU $2000-$2007 memory registers and copies */
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
ppu_scanline(ppu_t *ppu)
{
  //printf("PPU: scanline: %d\n", ppu->scanline);
  ppu->scanline++;
  if (ppu->scanline == 240) {
    SDL_RenderPresent(renderer);
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
    SDL_RenderClear(renderer);

#if 1
    printf("PPU frame: #%d\n", ppu->framecount);

    printf("=====================================================\n");
  int addr = 0x2000;
  for (int y = 0; y < 16; y++) {
    printf("%02X: ", addr);
    for (int i = 0; i < 16; i++) {
      printf("%02X ", ppu->mem[addr + i]);
    }
    printf("\n");
    addr += 16;
  }
#endif
    ppu->framecount++;
  }

#if 0
  int pindex = 0;
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      SDL_SetRenderDrawColor(renderer,
                             palette[pindex] >> 16,
                             palette[pindex] >> 8,
                             palette[pindex] & 0xff,
                             255);
      SDL_RenderDrawPoint(renderer, x, y);
    }
  }
#endif
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
  SDL_Event e;
  bool quit = false;

  while (SDL_PollEvent(&e)) {
    //If user closes the window
    if (e.type == SDL_QUIT) {
      quit = true;
    }
  }

  if (quit) {
    exit(0);
  }

  for (int i = 0; i < cycles; i++) {
    ppu_cycle(ppu);
  }
}
