/* Simple iNes v1.0 format parser (.nes)
 */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "ines.h"

#define HEADER(ines) ines->header

ines_t*
ines_load(const char *filename)
{
    ines_t *ines;

    ines = malloc(sizeof(ines_t));
    ines->filename = filename;
    ines->fd = open(filename, O_RDONLY);
    if (ines->fd == -1) {
       perror("Cannot open file");
       return NULL;
    }

    /* FIXME: Calculate the size of the file */
    ines->map = mmap(0, 40 * 1024, PROT_READ, MAP_SHARED, ines->fd, 0);

    assert(sizeof(header_t) == 16);
    memcpy(&ines->header, ines->map, sizeof(header_t));
    if (HEADER(ines).constant[0] != 'N' &&
        HEADER(ines).constant[1] != 'E' &&
        HEADER(ines).constant[2] != 'S' &&
        HEADER(ines).constant[3] != '\0') {
        perror("Invalid header");
        return NULL;
    }

    ines->prg = ines->map + sizeof(header_t);
    ines->chr = ines->prg + HEADER(ines).prg_size;

    ines_dump(ines);
    return ines;
}

void
ines_dump(ines_t* ines)
{
    printf("Constants: %c%c%c\n",
           HEADER(ines).constant[0],
           HEADER(ines).constant[1],
           HEADER(ines).constant[2]);

    printf("PRG size: %dkB\n", HEADER(ines).prg_size * 16);
    printf("CHR size: %dkB\n", HEADER(ines).chr_size * 8);
    printf("Mapper: %d\n", HEADER(ines).mapper);
}

uint16_t 
ines_prg_size(ines_t *ines)
{
  return HEADER(ines).prg_size * 16;
}

uint16_t 
ines_chr_size(ines_t *ines)
{
  return HEADER(ines).chr_size * 8;
}

void
ines_destroy(ines_t* ines)
{
    close(ines->fd); 
    free(ines);
}
