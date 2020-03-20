#ifndef __GBABUS_H__
#define __GBABUS_H__

#include "common/util.h"
#include "gbamem.h"
#include "arm7tdmi/arm7tdmi.h"
#include "ppu.h"

void init_gbabus(gbamem_t* new_mem, arm7tdmi_t* new_cpu, gba_ppu_t* new_ppu);
byte gba_read_byte(word addr);
half gba_read_half(word address);
void gba_write_byte(word addr, byte value);
void gba_write_half(word address, half value);
word gba_read_word(word address);
void gba_write_word(word address, word value);
#endif
