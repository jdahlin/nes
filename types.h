#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>

typedef struct emu_t emu_t;
typedef struct cpu_t cpu_t;
typedef struct ppu_t ppu_t;

struct emu_t {
  cpu_t *cpu;
  ppu_t *ppu;
};

struct cpu_t {
  /* Memory, 16bit addresses */
  uint8_t *mem;

  /* Program Counter */
  uint16_t pc;

  /* Accumulator register */
  uint8_t a;

  /* X register */
  uint8_t x;

  /* Y register */
  uint8_t y;

  /* Stack Pointer */
  uint8_t sp;

  /* Processor Status Words */
  struct {
    uint8_t c : 1; // Carry flag
    uint8_t z : 1; // Zero flag
    uint8_t i : 1; // Interrupt flag
    uint8_t d : 1; // Decimal flag
    uint8_t b : 1; // Breakpoint
    uint8_t u : 1; // Unused flag
    uint8_t v : 1; // oVerflow flag
    uint8_t n : 1; // Negative flag
  } p;

  /* Instruction counter */
  uint32_t instructions;

  emu_t *emu;

};

struct ppu_t {
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
  emu_t *emu;
};


#endif /* __TYPES_H__ */
