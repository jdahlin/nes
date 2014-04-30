/* CPU Emulation of a Ricoh 2A02 (NTSC) based on a MOS 6502 CPU
 * Operates at 1.79Mhz
 */
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cpu.h"
#include "ppu.h"

#define NMI_ADDRESS 0xFFFA
#define DEBUG_ASM 0

cpu_t*
cpu_create(emu_t *emu)
{
    cpu_t *cpu;
    cpu = calloc(sizeof(cpu_t), 1);
    cpu->mem = calloc(sizeof(uint8_t), 65536);
    cpu->emu = emu;
    return cpu;
}

void
cpu_map(cpu_t         *cpu,
        uint16_t       dest,
        const uint8_t *src,
        uint16_t       size)
{
    memcpy(&cpu->mem[dest], src, size);
}

static inline uint8_t
cpu_read_byte(cpu_t *cpu,
	      uint16_t addr)
{
  /* 0x2000..0x3fff is PPU and mirrors */
  if (addr >= 0x2000 && addr <= 0x3fff) {
    return ppu_read(cpu->emu->ppu, addr);
  } else if (addr >= 0x4000 && addr <= 0x401f) {
    //printf("FIXME: apu_read(%04X)\n", addr);
    return 0;
  /* Normal memory read */
  } else {
    return cpu->mem[addr];
  }
}

static inline uint16_t
cpu_read16(cpu_t *cpu,
	   uint16_t addr)
{
  uint16_t m;
  m = cpu_read_byte(cpu, addr);
  m |= cpu_read_byte(cpu, addr + 1) << 8;
  return m;
}

static inline void
cpu_write_byte(cpu_t    *cpu,
	       uint16_t  addr,
	       uint8_t   value)
{
  /* 0x2000..0x3fff is PPU and mirrors */
  if (addr >= 0x2000 && addr <= 0x3fff) {
    ppu_write(cpu->emu->ppu, addr, value);
  } else if (addr >= 0x4000 && addr <= 0x401f) {
    printf("FIXME: apu_write(%04X) = %02X\n", addr, value);
  } else if (addr >= 0xfffa) {
    printf("FIXME: write(%04X) = %02X\n", addr, value);
  /* Normal memory write */
  } else {
    cpu->mem[addr] = value;
  }
}

static inline uint8_t
cpu_next8(cpu_t *cpu)
{
  return cpu_read_byte(cpu, cpu->pc++);
}

static inline uint16_t
cpu_next16(cpu_t *cpu)
{
  uint16_t word;
  word = cpu_next8(cpu);
  word |= cpu_next8(cpu) << 8;
  return word;
}

#if DEBUG_ASM
static void
cpu_printf(cpu_t *cpu, int n, const char *str, ...)
{
  va_list args;
  char format[80];
  char tmp[5];

  va_start(args, str);
  snprintf(format, 8, "$%X: ", cpu->pc - n);
  for (int i = n; i > 0; i--) {
    snprintf(tmp, 4, "%02X ", cpu->mem[cpu->pc - i]);
    strncat(format, tmp, 4);
  }
  for (int i = 0; i < 10 - n * 3; i++) {
    strncat(format, " ", 1);
  }
  strncat(format, str, strlen(str));
  vprintf(format, args);
  va_end(args);
}
#else
static void
cpu_printf(cpu_t *cpu __attribute__((unused)),
	   int n __attribute__((unused)),
	   const char *str __attribute__((unused)), ...)
{
}
#endif

static inline uint16_t
cpu_wrap_add(uint16_t a, uint16_t b)
{
  uint16_t res = a + b;
  if ((a & 0xff) + b >= 0x100)
    res &= ~0x100;
  return res;
}

void
cpu_cycle(cpu_t *cpu)
{
  uint8_t next = cpu_next8(cpu);
  //printf("%02X (pc=%04x)\n", next, cpu->pc);
  switch(next) {
    case 0x09: { // ORA, immediate
      uint8_t m = cpu_next8(cpu);
      cpu->a |= m;
      cpu->p.n = (cpu->a >> 7) & 1;
      cpu->p.z = (cpu->a == 0) ? 1 : 0;
      cpu_printf(cpu, 2, "ORA #$%02X\n", m);
      break;
    }
    case 0x0A: { // ASL, accumulator
      uint8_t b;
      cpu->p.c = (cpu->a >> 7) & 1;
      b = (cpu->a << 1) & 0xFE;
      cpu->p.n = (b >> 7) & 1;
      cpu->p.z = (b == 0) ? 1 : 0;
      cpu_printf(cpu, 1, "ASL A\n");
      break;
    }
    case 0x10: { // BPL, relative
      uint8_t m = cpu_next8(cpu);
      uint16_t addr = cpu_wrap_add(cpu->pc, m);
      cpu_printf(cpu, 2, "BPL $%04X (?%x)\n", addr, cpu->p.n == 0);
      if (cpu->p.n == 0) cpu->pc = addr;
      break;
    }
    case 0x20: { // JSR
      uint16_t addr = cpu_next16(cpu);
      uint16_t t = cpu->pc - 1;
      cpu->mem[0x100 | cpu->sp--] = t >> 8;
      cpu->mem[0x100 | cpu->sp--] = t & 0xFF;
      cpu_printf(cpu, 3, "JSR $%04X\n", addr);
      cpu->pc = addr;
      break;
    }
    case 0x29: { // AND, immediate
      uint8_t m = cpu_next8(cpu);
      cpu->a &= m;
      cpu->p.n = (cpu->a >> 7) & 1;
      cpu->p.z = (cpu->a == 0) ? 1 : 0;
      cpu_printf(cpu, 2, "AND #$%02X\n", m);
      break;
    }
    case 0x2C: { // BIT, absolute
      uint16_t m = cpu_next16(cpu);
      uint16_t t = cpu->a & m;
      cpu->p.n = (t >> 7) & 1;
      cpu->p.v = (t >> 6) & 1;
      cpu->p.z = (t == 0) ? 1 : 0;
      cpu_printf(cpu, 3, "BIT $%04X = #&02X\n", m, cpu->a);
      break;
    }
    case 0x38: { // SEC, implied
      cpu->p.c = 1;
      cpu_printf(cpu, 1, "SEC\n");
      break;
    }
    case 0x48: { // PHA, accumulator
      cpu->mem[0x100 | cpu->sp--] = cpu->a;
      cpu_printf(cpu, 1, "PHA\n");
      break;
    }
    case 0x4A: { // LSR, accumulator
      uint8_t b = cpu->a;
      cpu->p.n = 0;
      b = (b >> 1) & 0x7F;
      cpu->p.z = (b == 0) ? 1 : 0;
      cpu_printf(cpu, 1, "LSR A\n");
      break;
    }
    case 0x4C: { // JMP, absolute
      uint16_t m = cpu_next16(cpu);
      cpu_printf(cpu, 3, "JMP $%04X\n", m);
      cpu->pc = m;
      break;
    }
    case 0x60: { // RTS
      uint16_t m;
      m = cpu->mem[0x100 | ++cpu->sp];
      m |= cpu->mem[0x100 | ++cpu->sp] << 8;
      cpu_printf(cpu, 1, "RTS -------------------\n");
      cpu->pc = m + 1;
      break;
    }
    case 0x65: { // ADC, zero page
      uint16_t t, m = cpu_next8(cpu);
      t = cpu->a + m + cpu->p.c;
      cpu->p.v = (((cpu->a >> 7) & 1) != ((cpu->a >> 7) & 1)) ? 1 : 0;
      cpu->p.n = (cpu->a >> 7) & 1;
      cpu->p.z = (t == 0) ? 1 : 0;
      if (cpu->p.d) {
	/* Normally A and M should be BCD() in a 6502, but
	 * Ricoh 2A03 used in a NES has decimal mode disabled
	 */
	t = cpu->a + m + cpu->p.c;
	cpu->p.c = (t > 99) ? 1:0;
      } else
	cpu->p.c = (t > 255) ? 1:0;
      cpu->a = t & 0xFF;
      cpu_printf(cpu, 2, "ADC $%04X = #$%02X\n", m, cpu->a);
      break;
    }
    case 0x68: { // PLA
      cpu->a = cpu->mem[0x100 | ++cpu->sp];
      cpu->p.n = (cpu->a >> 7) & 1;
      cpu->p.z = (cpu->a == 0) ? 1 : 0;
      cpu_printf(cpu, 1, "PLA\n");
      break;
    }
    case 0x78: { // SEI
      cpu->p.i = 1;
      cpu_printf(cpu, 1, "SEI\n");
      break;
    }
    case 0x85: { // STA, zero page
      uint16_t addr = cpu_next8(cpu);
      cpu_write_byte(cpu, addr, cpu->a);
      cpu_printf(cpu, 2, "STA $%04X = #$%02X\n", addr, cpu->a);
      break;
    }
    case 0x86: { // STX, zero page
      uint16_t addr = cpu_next8(cpu);
      cpu_write_byte(cpu, addr, cpu->x);
      cpu_printf(cpu, 2, "STX $%04X = #$%02X\n", addr, cpu->x);
      break;
    }
    case 0x88: { // DEY
      cpu->y--;
      cpu->p.z = (cpu->y == 0) ? 1 : 0;
      cpu->p.n = (cpu->y >> 7) & 1;
      cpu_printf(cpu, 1, "DEY\n");
      break;
    }
    case 0x8A: { // TXA
      cpu->a = cpu->x;
      cpu_printf(cpu, 1, "TXA\n");
      break;
    }
    case 0x8D: { // STA
      uint16_t m = cpu_next16(cpu);
      cpu_printf(cpu, 3, "STA $%04X = #%02X\n", m, cpu->a);
      cpu_write_byte(cpu, m, cpu->a);
      break;
    }
    case 0x90: { // BCC, relative
      uint8_t m = cpu_next8(cpu);
      uint16_t addr = cpu_wrap_add(cpu->pc, m);
      cpu_printf(cpu, 2, "BCC $%04X (?%x)\n", addr, cpu->p.c == 0);
      if (cpu->p.c == 0) cpu->pc = addr;
      break;
    }
    case 0x91: { // STA, indirect, Y
      uint16_t m = cpu_next8(cpu);
      cpu_printf(cpu, 2, "STA ($%02X),Y @ $%04X = #&%02X\n",
		 m, cpu->y, cpu->a);
      cpu_write_byte(cpu, m, cpu->y);
      break;
    }
    case 0x9A: { // TXS
      cpu->sp = cpu->x;
      cpu_printf(cpu, 1, "TXS\n");
      break;
    }
    case 0x98: { // TYA
      cpu->a = cpu->y;
      cpu_printf(cpu, 1, "TYA\n");
      break;
    }
    case 0x99: { // STA, absolute, y
      uint16_t m = cpu_next16(cpu);
      cpu_printf(cpu, 3, "STA #&%04X,Y\n", cpu->y);
      cpu_write_byte(cpu, m, cpu->y);
      break;
    }
    case 0xA0: { // LDY, immediate
      cpu->y = cpu_next8(cpu);
      cpu->p.n = (cpu->y >> 7) & 1;
      cpu->p.z = (cpu->y == 0) ? 1 : 0;
      cpu_printf(cpu, 2, "LDY #&%02X\n", cpu->y);
      break;
    }
    case 0xA2: { // LDX, immediate
      cpu->x = cpu_next8(cpu);
      cpu->p.n = (cpu->x >> 7) & 1;
      cpu->p.z = (cpu->x == 0) ? 1 : 0;
      cpu_printf(cpu, 2, "LDX #&%02X\n", cpu->x);
      break;
    }
    case 0xA8: { // TAY, implied
      cpu->y = cpu->a;
      cpu_printf(cpu, 1, "TAY\n");
      break;
    }
    case 0xA9: { // LDA, immediate
      cpu->a = cpu_next8(cpu);
      cpu->p.n = (cpu->a >> 7) & 1;
      cpu->p.z = (cpu->a == 0) ? 1 : 0;
      cpu_printf(cpu, 2, "LDA #&%02X\n", cpu->a);
      break;
    }
    case 0xAA: { // TAX, implied
      cpu->x = cpu->a;
      cpu_printf(cpu, 1, "TAX\n");
      break;
    }
    case 0xAC: { // LDY, absolute
      uint16_t addr = cpu_next16(cpu);
      cpu->y = cpu_read_byte(cpu, addr);
      cpu->p.n = (cpu->y >> 7) & 1;
      cpu->p.z = (cpu->y == 0) ? 1 : 0;
      cpu_printf(cpu, 3, "LDY $%04X = #&%02X (n=%d)\n", addr, cpu->y, cpu->p.n);
      break;
    }
    case 0xAD: { // LDA, absolute
      uint16_t addr = cpu_next16(cpu);
      cpu->a = cpu_read_byte(cpu, addr);
      cpu->p.n = (cpu->a >> 7) & 1;
      cpu->p.z = (cpu->a == 0) ? 1 : 0;
      cpu_printf(cpu, 3, "LDA $%04X = #&%02X (n=%d)\n", addr, cpu->a, cpu->p.n);
      break;
    }
    case 0xAE: { // LDX, absolute
      uint16_t addr = cpu_next16(cpu);
      cpu->x = cpu_read_byte(cpu, addr);
      cpu->p.n = (cpu->x >> 7) & 1;
      cpu->p.z = (cpu->x == 0) ? 1 : 0;
      cpu_printf(cpu, 3, "LDX $%04X = #&%02X (n=%d)\n", addr, cpu->x, cpu->p.n);
      break;
    }
    case 0xB0: { // BCS, relative
      uint16_t m = cpu_next8(cpu);
      uint16_t addr = cpu_wrap_add(cpu->pc, m);
      cpu_printf(cpu, 2, "BCS $%04X (?%d)\n", addr, cpu->p.c == 1);
      if (cpu->p.c == 1) cpu->pc = addr;
      break;
    }
    case 0xB1: { // LDA, indirect, Y
      uint8_t addr = cpu_next8(cpu);
      uint16_t t = cpu_read16(cpu, addr);
      // FIXME: wrong here some where.
      cpu->a = t;
      cpu->p.n = (t >> 7) & 1;
      cpu->p.z = (t == 0) ? 1 : 0;
      cpu_printf(cpu, 2, "LDA ($%02X),Y @ $%04X = #&%02X\n",
      		 addr, t, cpu->a);
      break;
    }
    case 0xBD: { // LDA, absolute, X
      uint16_t addr = cpu_next16(cpu);
      cpu->a = cpu_read_byte(cpu, addr);
      cpu->p.n = (cpu->a >> 7) & 1;
      cpu->p.z = (cpu->a == 0) ? 1 : 0;
      cpu_printf(cpu, 3, "LDA $%04X,X = #&%02X\n", addr, cpu->a);
      break;
    }
    case 0xC0: { // CPY, immediate
      uint16_t m = cpu_next8(cpu);
      uint16_t t = cpu->y - m;
      cpu->p.n = (t >> 7) & 1;
      cpu->p.c = (cpu->y >= m) ? 1 : 0;
      cpu->p.z = (t == 0) ? 1 : 0;
      cpu_printf(cpu, 2, "CPY #$%02X\n", m);
      break;
    }
    case 0xC8: { // INY
      cpu->y++;
      cpu->p.z = (cpu->y == 0) ? 1 : 0;
      cpu->p.n = (cpu->y >> 7) & 1;
      cpu_printf(cpu, 1, "INY\n");
      break;
    }
    case 0xCA: { // DEX
      cpu->x--;
      cpu->p.z = (cpu->x == 0) ? 1 : 0;
      cpu->p.n = (cpu->x >> 7) & 1;
      cpu_printf(cpu, 1, "DEX\n");
      break;
    }
    case 0xC9: { // CMP, immediate
      uint16_t m = cpu_next8(cpu);
      uint16_t t = cpu->a - m;
      cpu->p.n = (t >> 7) & 1;
      cpu->p.c = (cpu->a >= m) ? 1 : 0;
      cpu->p.z = (t == 0) ? 1 : 0;
      cpu_printf(cpu, 2, "CMP #&%02X\n", m);
      break;
    }
    case 0xD0: { // BNE, relative
      uint8_t m = cpu_next8(cpu);
      uint16_t addr = cpu_wrap_add(cpu->pc, m);
      cpu_printf(cpu, 2, "BNE $%04X (?%d)\n", addr, cpu->p.z == 0);
      if (cpu->p.z == 0) cpu->pc = addr;
      break;
    }
    case 0xD8: { // CLD
      cpu->p.d = 0;
      cpu_printf(cpu, 1, "CLD\n");
      break;
    }
    case 0xE0: { // CPX, immediate
      uint16_t m = cpu_next8(cpu);
      uint16_t t = cpu->x - m;
      cpu->p.n = (t >> 7) & 1;
      cpu->p.c = (cpu->x >= m) ? 1 : 0;
      cpu->p.z = (t == 0) ? 1 : 0;
      cpu_printf(cpu, 2, "CPX #&%02X\n", m);
      break;
    }
    case 0xEE: { // INC, absolute
      uint16_t addr = cpu_next16(cpu);
      uint16_t value = cpu_read_byte(cpu, addr);
      uint16_t m = (value + 1) & 0xff;
      cpu_write_byte(cpu, addr, m);
      cpu->p.n = (m >> 7) & 1;
      cpu->p.z = (m == 0) ? 1 : 0;
      cpu_printf(cpu, 3, "INC $%04X = #&%02X\n", addr, m);
      break;
    }
    default:
      cpu_printf(cpu, 1, "OPCODE $%02X not implemented\n", next);
      exit(-1);
      break;
  }
}

void
cpu_nmi_handler(cpu_t *cpu)
{
  uint16_t addr;
  addr = cpu_read16(cpu, NMI_ADDRESS);

  printf("NMI handler: %04X\n", addr);
  if (addr) {
    cpu->pc = addr;
  }
  ppu_nmi_disable(cpu->emu->ppu);
}

void
cpu_run(cpu_t *cpu,
	uint16_t address)
{
    cpu->pc = address;
    while (1) {
      bool nmi = ppu_nmi_is_enabled(cpu->emu->ppu);
      cpu_cycle(cpu);
      if (nmi) {
	cpu_nmi_handler(cpu);
      }
      ppu_run(cpu->emu->ppu, 4);
    }
}
