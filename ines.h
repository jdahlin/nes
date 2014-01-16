#ifndef __INES_H__
#define __INES_H__

#include <stdint.h>

typedef struct {
  char constant[4]; /* 'N' 'E' 'S' '\0' */
  uint8_t prg_size;
  uint8_t chr_size;
  char mirror : 1;
  char battery : 1;
  char trainer : 1;
  char unused : 1;
  char mapper : 4;
  char vs : 1;
  char playchoise : 1;
  char v2 : 2;
  char mapper_upper;
  uint8_t prg_ram_size;
  char reserved[6];
} header_t;


typedef struct {
  const char *filename;
  int fd;
  const void *map;
  header_t header;
  const uint8_t *prg;
  const uint8_t *chr;
} ines_t;

ines_t * ines_load(const char *filename);
void ines_dump(ines_t* ines);
void ines_destroy(ines_t* ines);
uint16_t ines_prg_size(ines_t *ines);
uint16_t ines_chr_size(ines_t *ines);

#endif /* __INES_H__ */
