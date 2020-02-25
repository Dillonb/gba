#ifndef __GBABUS_H__
#define __GBABUS_H__

#include "common/util.h"
#include "gbamem.h"

void init_gbabus(gbamem_t* new_mem);
byte gba_read_byte(word addr);
half gba_read_half(word addr);
void gba_write_byte(word addr, byte value);
void gba_write_half(word address, half value);
word gba_read_word(word addr);
void gba_write_word(word address, word value);
#endif
