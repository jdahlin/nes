#ifndef __CPU_H__
#define __CPU_H__

#include <stdint.h>
#include "ppu.h"

typedef struct {
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
  union {
    uint8_t c : 1; // Carry flag
    uint8_t z : 1; // Zero flag
    uint8_t i : 1; // Interrupt flag
    uint8_t d : 1; // Decimal flag
    uint8_t b : 1; // Breakpoint
    uint8_t _ : 1; // -
    uint8_t v : 1; // oVerflow flag
    uint8_t n : 1; // Negative flag
  } p;
  ppu_t *ppu;
} cpu_t;

cpu_t* cpu_create (void);
void cpu_map(cpu_t   *cpu,
	     uint16_t dest,
	     const uint8_t *src,
	     uint16_t  size);
void cpu_run(cpu_t *cpu,
	     uint16_t address);

#endif /* __CPU_H__ */
