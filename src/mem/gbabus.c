#include <string.h>

#include "gbabus.h"
#include "ioreg_util.h"
#include "ioreg_names.h"
#include "gbabios.h"
#include "../gba_system.h"
#include "backup/flash.h"


INLINE word open_bus(word addr);

#define REGION_BIOS       0x00
#define REGION_EWRAM      0x02
#define REGION_IWRAM      0x03
#define REGION_IOREG      0x04
#define REGION_PRAM       0x05
#define REGION_VRAM       0x06
#define REGION_OAM        0x07
#define REGION_GAMEPAK0_L 0x08
#define REGION_GAMEPAK0_H 0x09
#define REGION_GAMEPAK1_L 0x0A
#define REGION_GAMEPAK1_H 0x0B
#define REGION_GAMEPAK2_L 0x0C
#define REGION_GAMEPAK2_H 0x0D
#define REGION_SRAM       0x0E
#define REGION_SRAM_MIRR  0x0F

gbabus_t* init_gbabus() {
    gbabus_t* bus_state = malloc(sizeof(gbabus_t));
    bus_state->interrupt_master_enable.raw = 0;
    bus_state->interrupt_enable.raw = 0;
    bus_state->KEYINPUT.raw = 0xFFFF;
    bus_state->SOUNDBIAS.raw = 0x0200;

    bus_state->DMA0INT.previously_enabled = false;
    bus_state->DMA1INT.previously_enabled = false;
    bus_state->DMA2INT.previously_enabled = false;
    bus_state->DMA3INT.previously_enabled = false;

    // Loop through every single word aligned address in the ROM and try to find what backup type it is (you read that right)
    // Start at 0xE4 since everything before that is part of the header
    for (int addr = 0xE4; addr < (mem->rom_size - 4); addr += 4) {
        if (memcmp("SRAM", &mem->rom[addr], 4) == 0) {
            bus_state->backup_type = SRAM;
            logwarn("Determined backup type: SRAM")
            mem->backup = malloc(SRAM_SIZE);
            mem->backup_size = SRAM_SIZE;
            memset(mem->backup, 0, SRAM_SIZE);
            break;
        }
        if (memcmp("EEPROM", &mem->rom[addr], 6) == 0) {
            bus_state->backup_type = EEPROM;
            logfatal("Determined backup type: EEPROM")
            break;
        }
        if (memcmp("FLASH_", &mem->rom[addr], 6) == 0) {
            bus_state->backup_type = FLASH64K;
            logfatal("Determined backup type: FLASH64K")
            break;
        }
        if (memcmp("FLASH512_", &mem->rom[addr], 9) == 0) {
            bus_state->backup_type = FLASH64K;
            init_flash(mem, FLASH64K);
            break;
        }
        if (memcmp("FLASH1M_", &mem->rom[addr], 8) == 0) {
            bus_state->backup_type = FLASH128K;
            init_flash(mem, FLASH128K);
            break;
        }
    }

    return bus_state;
}

KEYINPUT_t* get_keyinput() {
    return &bus->KEYINPUT;
}

void request_interrupt(gba_interrupt_t interrupt) {
    if (bus->interrupt_master_enable.enable) {
        switch (interrupt) {
            case IRQ_VBLANK:
                if (bus->interrupt_enable.lcd_vblank) {
                    cpu->irq = true;
                    bus->IF.vblank = true;
                } else {
                    logwarn("VBlank interrupt blocked by IE")
                }
                break;
            case IRQ_HBLANK:
                if (bus->interrupt_enable.lcd_hblank) {
                    cpu->irq = true;
                    bus->IF.hblank = true;
                } else {
                    logwarn("HBlank interrupt blocked by IE")
                }
                break;
            case IRQ_VCOUNT:
                if (bus->interrupt_enable.lcd_vcounter_match) {
                    cpu->irq = true;
                    bus->IF.vcount = true;
                } else {
                    logwarn("VCount interrupt blocked by IE")
                }
                break;
            case IRQ_TIMER0:
                if (bus->interrupt_enable.timer0_overflow) {
                    cpu->irq = true;
                    bus->IF.timer0 = true;
                } else {
                    logwarn("Timer0 overflow interrupt blocked by IE")
                }
                break;
            case IRQ_TIMER1:
                if (bus->interrupt_enable.timer1_overflow) {
                    cpu->irq = true;
                    bus->IF.timer1 = true;
                } else {
                    logwarn("Timer1 overflow interrupt blocked by IE")
                }
                break;
            case IRQ_TIMER2:
                if (bus->interrupt_enable.timer2_overflow) {
                    cpu->irq = true;
                    bus->IF.timer2 = true;
                } else {
                    logwarn("Timer2 overflow interrupt blocked by IE")
                }
                break;
            case IRQ_TIMER3:
                if (bus->interrupt_enable.timer3_overflow) {
                    cpu->irq = true;
                    bus->IF.timer3 = true;
                } else {
                    logwarn("Timer3 overflow interrupt blocked by IE")
                }
                break;
            case IRQ_DMA0:
                if (bus->interrupt_enable.dma_0) {
                    cpu->irq = true;
                    bus->IF.dma0 = true;
                } else {
                    logwarn("DMA0 interrupt blocked by IE")
                }
                break;
            case IRQ_DMA1:
                if (bus->interrupt_enable.dma_1) {
                    cpu->irq = true;
                    bus->IF.dma1 = true;
                } else {
                    logwarn("DMA1 interrupt blocked by IE")
                }
                break;
            case IRQ_DMA2:
                if (bus->interrupt_enable.dma_2) {
                    cpu->irq = true;
                    bus->IF.dma2 = true;
                } else {
                    logwarn("DMA2 interrupt blocked by IE")
                }
                break;
            case IRQ_DMA3:
                if (bus->interrupt_enable.dma_3) {
                    cpu->irq = true;
                    bus->IF.dma3 = true;
                } else {
                    logwarn("DMA3 interrupt blocked by IE")
                }
                break;
            default:
                logfatal("Unknown interrupt index %d requested!", interrupt)
        }
    } else {
        logwarn("Interrupt blocked by IME")
    }
}

INLINE void write_half_ioreg_masked(word addr, half value, half mask);
INLINE void write_word_ioreg_masked(word addr, word value, word mask);

INLINE void write_byte_ioreg(word addr, byte value) {
    if (!is_ioreg_writable(addr)) {
        logwarn("Ignoring write to unwriteable byte ioreg 0x%08X", addr)
        return;
    }
    byte size = get_ioreg_size_for_addr(addr);
    if (size == sizeof(half)) {
        word offset = (addr % sizeof(half));
        half shifted_value = value;
        shifted_value <<= (offset * 8);
        write_half_ioreg_masked(addr - offset, shifted_value, 0xFF << (offset * 8));
    } else if (size == sizeof(word)) {
        word offset = (addr % sizeof(word));
        word shifted_value = value;
        shifted_value <<= (offset * 8);
        write_word_ioreg_masked(addr - offset, shifted_value, 0xFF << (offset * 8));
    } else {
        // Don't really need to do the get addr/write masked thing that the bigger ioregs do
        // Or do I?
        word regnum = addr & 0xFFF;
        switch (regnum) {
            case IO_HALTCNT: {
                if ((value & 1) == 0) {
                    logwarn("HALTING CPU!")
                    cpu->halt = true;
                } else {
                    logfatal("Wrote to HALTCNT with bit 0 being 1")
                }
                break;
            }
            case IO_POSTFLG:
                logwarn("Ignoring write to POSTFLG register")
                break;
            default:
                logfatal("Write to unknown (but valid) byte ioreg addr 0x%08X == 0x%02X", addr, value)
        }
    }
}

INLINE byte read_byte_ioreg(word addr) {
    switch (addr & 0xFFF) {
        case IO_POSTFLG:
            logwarn("Ignoring read from POSTFLG reg. Returning 0")
            return 0;
        default:
            logfatal("Reading byte ioreg at 0x%08X", addr)
    }
}

INLINE half* get_half_ioreg_ptr(word addr, bool write) {
    word regnum = addr & 0xFFF;
    switch (regnum) {
        case IO_DISPCNT: return &ppu->DISPCNT.raw;
        case IO_DISPSTAT: return &ppu->DISPSTAT.raw;
        case IO_BG0CNT: return &ppu->BG0CNT.raw;
        case IO_BG1CNT: return &ppu->BG1CNT.raw;
        case IO_BG2CNT: return &ppu->BG2CNT.raw;
        case IO_BG3CNT: return &ppu->BG3CNT.raw;
        case IO_BG0HOFS: return &ppu->BG0HOFS.raw;
        case IO_BG1HOFS: return &ppu->BG1HOFS.raw;
        case IO_BG2HOFS: return &ppu->BG2HOFS.raw;
        case IO_BG3HOFS: return &ppu->BG3HOFS.raw;
        case IO_BG0VOFS: return &ppu->BG0VOFS.raw;
        case IO_BG1VOFS: return &ppu->BG1VOFS.raw;
        case IO_BG2VOFS: return &ppu->BG2VOFS.raw;
        case IO_BG3VOFS: return &ppu->BG3VOFS.raw;
        case IO_BG2PA: return &ppu->BG2PA.raw;
        case IO_BG2PB: return &ppu->BG2PB.raw;
        case IO_BG2PC: return &ppu->BG2PC.raw;
        case IO_BG2PD: return &ppu->BG2PD.raw;
        case IO_BG3PA: return &ppu->BG3PA.raw;
        case IO_BG3PB: return &ppu->BG3PB.raw;
        case IO_BG3PC: return &ppu->BG3PC.raw;
        case IO_BG3PD: return &ppu->BG3PD.raw;
        case IO_WIN0H: return &ppu->WIN0H.raw;
        case IO_WIN1H: return &ppu->WIN1H.raw;
        case IO_WIN0V: return &ppu->WIN0V.raw;
        case IO_WIN1V: return &ppu->WIN1V.raw;
        case IO_WININ: return &ppu->WININ.raw;
        case IO_WINOUT: return &ppu->WINOUT.raw;
        case IO_MOSAIC: return &ppu->MOSAIC.raw;
        case IO_BLDCNT: return &ppu->BLDCNT.raw;
        case IO_BLDALPHA: return &ppu->BLDALPHA.raw;
        case IO_BLDY: return &ppu->BLDY.raw;
        case IO_IE: return &bus->interrupt_enable.raw;
        case IO_DMA0CNT_L: return &bus->DMA0CNT_L.raw;
        case IO_DMA0CNT_H: return &bus->DMA0CNT_H.raw;
        case IO_DMA1CNT_L: return &bus->DMA1CNT_L.raw;
        case IO_DMA1CNT_H: return &bus->DMA1CNT_H.raw;
        case IO_DMA2CNT_L: return &bus->DMA2CNT_L.raw;
        case IO_DMA2CNT_H: return &bus->DMA2CNT_H.raw;
        case IO_DMA3CNT_L: return &bus->DMA3CNT_L.raw;
        case IO_DMA3CNT_H: return &bus->DMA3CNT_H.raw;
        case IO_KEYINPUT: return &bus->KEYINPUT.raw;
        case IO_RCNT: return &bus->RCNT.raw;
        case IO_JOYCNT: return &bus->JOYCNT.raw;
        case IO_IME: return &bus->interrupt_master_enable.raw;
        case IO_SOUNDBIAS: return &bus->SOUNDBIAS.raw;
        case IO_TM0CNT_L: {
            if (write) {
                return &bus->TMCNT_L[0].raw;
            } else {
                return &bus->TMINT[0].value;
            }
        }
        case IO_TM0CNT_H: return &bus->TMCNT_H[0].raw;
        case IO_TM1CNT_L: {
            if (write) {
                return &bus->TMCNT_L[1].raw;
            } else {
                return &bus->TMINT[1].value;
            }
        }
        case IO_TM1CNT_H: return &bus->TMCNT_H[1].raw;
        case IO_TM2CNT_L: {
            if (write) {
                return &bus->TMCNT_L[2].raw;
            } else {
                return &bus->TMINT[2].value;
            }
        }
        case IO_TM2CNT_H: return &bus->TMCNT_H[2].raw;
        case IO_TM3CNT_L: {
            if (write) {
                return &bus->TMCNT_L[3].raw;
            } else {
                return &bus->TMINT[3].value;
            }
        }
        case IO_TM3CNT_H: return &bus->TMCNT_H[3].raw;
        case IO_KEYCNT: return &bus->KEYCNT.raw;
        case IO_IF: return &bus->IF.raw;
        case IO_WAITCNT:
            return &bus->WAITCNT.raw;
        case IO_UNDOCUMENTED_GREEN_SWAP:
            logwarn("Ignoring access to Green Swap register")
            return NULL;
        case IO_VCOUNT: return &ppu->y;
        case IO_SOUND1CNT_L:
        case IO_SOUND1CNT_H:
        case IO_SOUND1CNT_X:
        case IO_SOUND2CNT_L:
        case IO_SOUND2CNT_H:
        case IO_SOUND3CNT_L:
        case IO_SOUND3CNT_H:
        case IO_SOUND3CNT_X:
        case IO_SOUND4CNT_L:
        case IO_SOUND4CNT_H:
        case IO_SOUNDCNT_L:
        case IO_SOUNDCNT_H:
        case IO_SOUNDCNT_X:
        case WAVE_RAM0_L:
        case WAVE_RAM0_H:
        case WAVE_RAM1_L:
        case WAVE_RAM1_H:
        case WAVE_RAM2_L:
        case WAVE_RAM2_H:
        case WAVE_RAM3_L:
        case WAVE_RAM3_H:
            logwarn("Ignoring access to sound register: 0x%03X", regnum)
            return NULL;

        case IO_SIOCNT:
        case IO_SIOMULTI0:
        case IO_SIOMULTI1:
        case IO_SIOMULTI2:
        case IO_SIOMULTI3:
        case IO_SIOMLT_SEND:
        case IO_JOY_RECV:
        case IO_JOY_TRANS:
        case IO_JOYSTAT:
            logwarn("Ignoring access to SIO register: 0x%03X", regnum)
            return NULL;

        default:
            logfatal("Access to unknown (but valid) half ioreg addr 0x%08X", addr)
    }
}

INLINE void write_half_ioreg_masked(word addr, half value, half mask) {
    half* ioreg = get_half_ioreg_ptr(addr, true);
    if (ioreg) {
        switch (addr & 0xFFF) {
            case IO_DISPSTAT:
                mask &= 0b1111111111111000; // Last 3 bits are read-only
                break;
            case IO_IME:
                mask = 0b1;
                break;
            case IO_IF: {
                bus->IF.raw &= ~value;
                return;
            }
            default:
                break; // No special case
        }
        *ioreg &= (~mask);
        *ioreg |= (value & mask);
        switch (addr & 0xFFF) {
            case IO_DMA0CNT_H:
                if (!bus->DMA0CNT_H.dma_enable) {
                    bus->DMA0INT.previously_enabled = false;
                }
                break;
            case IO_DMA1CNT_H:
                if (!bus->DMA1CNT_H.dma_enable) {
                    bus->DMA1INT.previously_enabled = false;
                }
                break;
            case IO_DMA2CNT_H:
                if (!bus->DMA2CNT_H.dma_enable) {
                    bus->DMA2INT.previously_enabled = false;
                }
                break;
            case IO_DMA3CNT_H:
                if (!bus->DMA3CNT_H.dma_enable) {
                    bus->DMA3INT.previously_enabled = false;
                }
                break;
        }
    } else {
        logwarn("Ignoring write to half ioreg 0x%08X", addr)
    }
}

INLINE void write_half_ioreg(word addr, half value) {
    if (!is_ioreg_writable(addr)) {
        logwarn("Ignoring write to unwriteable half ioreg 0x%08X", addr)
        return;
    }
    // Write to the whole thing
    write_half_ioreg_masked(addr, value, 0xFFFF);
}

INLINE word* get_word_ioreg_ptr(word addr) {
    unimplemented(get_ioreg_size_for_addr(addr) != sizeof(word), "Trying to get the address of a wrong-sized word ioreg")
    switch (addr & 0xFFF) {
        case IO_BG2X:     return &ppu->BG2X.initial.raw;
        case IO_BG2Y:     return &ppu->BG2Y.initial.raw;
        case IO_BG3X:     return &ppu->BG3X.initial.raw;
        case IO_BG3Y:     return &ppu->BG3Y.initial.raw;
        case IO_DMA0SAD:  return &bus->DMA0SAD.raw;
        case IO_DMA0DAD:  return &bus->DMA0DAD.raw;
        case IO_DMA1SAD:  return &bus->DMA1SAD.raw;
        case IO_DMA1DAD:  return &bus->DMA1DAD.raw;
        case IO_DMA2SAD:  return &bus->DMA2SAD.raw;
        case IO_DMA2DAD:  return &bus->DMA2DAD.raw;
        case IO_DMA3SAD:  return &bus->DMA3SAD.raw;
        case IO_DMA3DAD:  return &bus->DMA3DAD.raw;
        case IO_FIFO_A:
        case IO_FIFO_B:
        case IO_JOY_RECV:
        case IO_JOY_TRANS:
        case IO_IMEM_CTRL:
            return NULL;
        default: logfatal("Tried to get the address of an unknown (but valid) word ioreg addr: 0x%08X", addr)
    }
}

INLINE void write_word_ioreg_masked(word addr, word value, word mask) {
    word maskedaddr = addr & 0xFFF;
    switch (maskedaddr) {
        case IO_FIFO_A:
            unimplemented(mask != 0xFFFFFFFF, "Write to FIFO not all at once")
            write_fifo(apu, 0, value);
            break;
        case IO_FIFO_B:
            unimplemented(mask != 0xFFFFFFFF, "Write to FIFO not all at once")
            write_fifo(apu, 1, value);
            break;
        case IO_BG2X:
        case IO_BG2Y:
        case IO_BG3X:
        case IO_BG3Y: {
            word* current = NULL;
            word* initial = NULL;
            switch (maskedaddr) {
                case IO_BG2X:
                    current = &ppu->BG2X.current.raw;
                    initial = &ppu->BG2X.initial.raw;
                    break;
                case IO_BG2Y:
                    current = &ppu->BG2Y.current.raw;
                    initial = &ppu->BG2Y.initial.raw;
                    break;
                case IO_BG3X:
                    current = &ppu->BG3X.current.raw;
                    initial = &ppu->BG3X.initial.raw;
                    break;
                case IO_BG3Y:
                    current = &ppu->BG3Y.current.raw;
                    initial = &ppu->BG3Y.initial.raw;
                    break;
                default:
                    logfatal("Fatal error- should not have ended up here.")
            }
            // Only update "current" if is VBlank
            if (!is_vblank(ppu)) {
                *current &= (~mask);
                *current |= (value & mask);
            }
            *initial &= (~mask);
            *initial |= (value & mask);
            break;
        }
        default:
            if (!is_ioreg_writable(addr)) {
                logwarn("Ignoring write to unwriteable word ioreg 0x%08X", addr)
                return;
            }
            word* ioreg = get_word_ioreg_ptr(addr);
            if (ioreg) {
                *ioreg &= (~mask);
                *ioreg |= (value & mask);
            } else {
                logwarn("Ignoring write to word ioreg 0x%08X with mask 0x%08X", addr, mask)
            }
    }
}

INLINE void write_word_ioreg(word addr, word value) {
    // 0x04XX0800 is the only address that's mirrored.
    if ((addr & 0xFF00FFFFu) == 0x04000800u) {
        addr = 0x04000800;
    }

    if (!is_ioreg_writable(addr)) {
        logwarn("Ignoring write to unwriteable word ioreg 0x%08X", addr)
    }

    write_word_ioreg_masked(addr, value, 0xFFFFFFFF);
}

INLINE word read_word_ioreg(word addr) {
    if (!is_ioreg_readable(addr)) {
        logwarn("Returning 0 (UNREADABLE BUT VALID WORD IOREG 0x%08X)", addr)
        return 0;
    }
    word* ioreg = get_word_ioreg_ptr(addr);
    if (ioreg) {
        return *ioreg;
    } else {
        logwarn("Ignoring read from word ioreg at 0x%08X and returning 0.", addr)
        return 0;
    }
}

INLINE half read_half_ioreg(word addr) {
    if (!is_ioreg_readable(addr)) {
        logwarn("Returning 0 (UNREADABLE BUT VALID HALF IOREG 0x%08X)", addr)
        return 0;
    }
    half* ioreg = get_half_ioreg_ptr(addr, false);
    if (ioreg) {
        return *ioreg;
    } else {
        logwarn("Ignoring read from half ioreg at 0x%08X and returning 0.", addr)
        return 0;
    }
}

INLINE bool is_open_bus(word address) {
    switch (address >> 24) {
        case 0x0:
            return address >= GBA_BIOS_SIZE;
        case 0x1:
            return true;
        case 0x2:
        case 0x3:
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
        case 0x8:
            return false;
        case 0x9:
        case 0xA:
        case 0xB:
        case 0xC:
            return (address & 0x1FFFFFF) >= mem->rom_size;
        case 0xD:
            unimplemented(bus->backup_type == EEPROM, "This region is different when the backup type is EEPROM")
            return (address & 0x1FFFFFF) >= mem->rom_size;
        case 0xE:
            if (bus->backup_type == FLASH64K || bus->backup_type == FLASH128K) {
                return (address & 0x1FFFFFF) >= mem->rom_size;
            } else if (bus->backup_type == EEPROM || bus->backup_type == SRAM) {
                return false;
            } else {
                logfatal("Unknown backup type %d", bus->backup_type)
            }
        case 0xF:
            return false; // Always
        default:
            return true;
    }
}

INLINE word open_bus(word addr) {
    word result;

    if (cpu->cpsr.thumb)
    {
        word low = cpu->pipeline[1];
        word high = cpu->pipeline[1];

        byte region = addr >> 24;

        if (region == 0 || region == 7) {
            low = cpu->pipeline[0];
        } else if (region == 3) {
            if (addr & 3) {
                low = cpu->pipeline[0];
            } else {
                high = cpu->pipeline[0];
            }
        }

        result = (high << 16) | low;
    } else {
        result = cpu->pipeline[1];
    }
    result >>= ((addr & 0b11u) << 3u);

    logwarn("RETURNING FROM OPEN BUS AT ADDRESS 0x%08X: 0x%08X", addr, result)
    return result;
}

INLINE byte inline_gba_read_byte(word addr) {
    addr &= ~(sizeof(byte) - 1);
    switch (addr >> 24) {
        case REGION_BIOS: {
            if (addr < GBA_BIOS_SIZE) { // BIOS
                return gbabios_read_byte(addr);
            } else {
                return open_bus(addr);
            }
        }
        case REGION_EWRAM: {
            word index = (addr - 0x02000000) % 0x40000;
            return mem->ewram[index];
        }
        case REGION_IWRAM: {
            word index = (addr - 0x03000000) % 0x8000;
            return mem->iwram[index];
        }
        case REGION_IOREG: {
            if (addr < 0x04000400) {
                byte size = get_ioreg_size_for_addr(addr);
                if (size == 0) {
                    logwarn("Returning open bus (UNUSED BYTE IOREG 0x%08X)", addr)
                    return open_bus(addr);
                } else if (size == sizeof(half)) {
                    int ofs = addr % 2;
                    half* ioreg = get_half_ioreg_ptr(addr - ofs, false);
                    if (ioreg) {
                        return (*ioreg >> ofs) & 0xFF;
                    } else {
                        logwarn("Ignoring read from half ioreg 0x%08X", addr - ofs)
                        return 0;
                    }
                }
                else if (size > sizeof(byte)) {
                    logfatal("Reading from too-large ioreg (%d) as byte at 0x%08X", size, addr)
                }
                if (!is_ioreg_readable(addr)) {
                    logwarn("Returning 0 (UNREADABLE BUT VALID BYTE IOREG 0x%08X)", addr)
                    return 0;
                }
                return read_byte_ioreg(addr);
            } else {
                logwarn("Tried to read from 0x%08X", addr)
                return open_bus(addr);
            }
        }
        case REGION_PRAM: {
            word index = (addr - 0x5000000) % 0x400;
            return ppu->pram[index];
        }
        case REGION_VRAM: {
            word index = addr - 0x06000000;
            index %= VRAM_SIZE;
            return ppu->vram[index];
        }
        case REGION_OAM: {
            word index = addr - 0x07000000;
            index %= OAM_SIZE;
            return ppu->oam[index];
        }
        case REGION_GAMEPAK0_L:
        case REGION_GAMEPAK0_H:
        case REGION_GAMEPAK1_L:
        case REGION_GAMEPAK1_H:
        case REGION_GAMEPAK2_L: {
            word index = addr & 0x1FFFFFF;
            if (index < mem->rom_size) {
                return mem->rom[index];
            } else {
                return open_bus(addr);
            }

        }

        case REGION_GAMEPAK2_H:
            if (bus->backup_type == EEPROM) {
                if (mem->rom_size <= 0x1000000 || addr >= 0xDFFFF00) {
                    return 1;
                }
            }
            word index = addr & 0x1FFFFFF;
            if (index < mem->rom_size) {
                return mem->rom[index];
            } else {
                return open_bus(addr);
            }

        case REGION_SRAM:
            switch (bus->backup_type) {
                case SRAM:
                    return mem->backup[addr & 0x7FFF];
                case FLASH64K:
                case FLASH128K:
                    return read_byte_flash(mem, addr, bus->backup_type);
                default: break;
            }
            return 0;

        case REGION_SRAM_MIRR:
            if (bus->backup_type == SRAM) {
                return mem->backup[addr & 0x7FFF];
            }
            return 0;
    }
    return open_bus(addr);
}

byte gba_read_byte(word addr) {
    return inline_gba_read_byte(addr);
}

INLINE half inline_gba_read_half(word address) {
    address &= ~(sizeof(half) - 1);
    if (is_ioreg(address)) {
        byte ioreg_size = get_ioreg_size_for_addr(address);
        unimplemented(ioreg_size > sizeof(half), "Reading from a too-large ioreg from gba_read_half")
        if (ioreg_size == sizeof(half)) {
            return read_half_ioreg(address);
        } else if (!is_ioreg_readable(address & ~(sizeof(word) - 1))) {
            return open_bus(address);
        } else if (ioreg_size == 0) {
            // Unused io register
            logwarn("Read from unused half size ioregister!")
            return 0;
        }
    }

    if (is_open_bus(address)) {
        return open_bus(address);
    }
    byte lower = inline_gba_read_byte(address);
    byte upper = inline_gba_read_byte(address + 1);

    return (upper << 8u) | lower;
}

half gba_read_half(word address) {
    return inline_gba_read_half(address);
}


void gba_write_byte(word addr, byte value) {
    addr &= ~(sizeof(byte) - 1);
    if (addr < GBA_BIOS_SIZE) {
        logwarn("Tried to write to the BIOS!")
    } else if (addr < 0x01FFFFFF) {
        logwarn("Tried to write to unused section of RAM in between bios and WRAM")
    } else if (addr < 0x03000000) { // EWRAM
        word index = (addr - 0x02000000) % 0x40000;
        mem->ewram[index] = value;
    } else if (addr < 0x04000000) { // IWRAM
        word index = (addr - 0x03000000) % 0x8000;
        mem->iwram[index] = value;
    } else if (addr < 0x04000400) {
        write_byte_ioreg(addr, value);
    } else if (addr < 0x05000000) {
        logwarn("Tried to write to 0x%08X", addr)
    } else if (addr < 0x06000000) { // Palette RAM
        word index = (addr - 0x5000000) % 0x400;
        ppu->pram[index] = value;
    } else if (addr < 0x07000000) {
        word index = addr & 0x1FFFF;
        if (index > 0x17FFF) {
            index -= 0x8000;
        }
        ppu->vram[index] = value;
    } else if (addr < 0x08000000) {
        word index = addr - 0x07000000;
        index %= OAM_SIZE;
        ppu->oam[index] = value;
    } else if (addr < 0x08000000 + mem->rom_size) {
        logwarn("Ignoring write to valid cartridge address 0x%08X!", addr)
    } else if ((addr >> 24) >= 0xE && addr < 0x10000000) {
        // Backup space
        switch (bus->backup_type) {
            case SRAM:
                mem->backup[addr & 0x7FFF] = value;
                break;
            case UNKNOWN:
                logwarn("Tried to access backup when backup type unknown!")
                break;
            case EEPROM:
                logfatal("Backup type EEPROM unimplemented!")
            case FLASH64K:
            case FLASH128K:
                write_byte_flash(mem, bus, addr, value);
                break;
            default:
                logfatal("Unknown backup type index %d!", bus->backup_type)
        }
    }
}

void gba_write_half(word address, half value) {
    address &= ~(sizeof(half) - 1);
    if (is_ioreg(address)) {
        byte ioreg_size = get_ioreg_size_for_addr(address);
        if (ioreg_size == sizeof(word)) {
            word offset = (address % sizeof(word));
            word shifted_value = value;
            shifted_value <<= (offset * 8);
            write_word_ioreg_masked(address - offset, shifted_value, 0xFFFF << (offset * 8));
        } else if (ioreg_size == sizeof(half)) {
            write_half_ioreg(address, value);
            return;
        } else if (ioreg_size == 0) {
            // Unused io register
            logwarn("Unused half size ioregister 0x%08X", address)
            return;
        }
    }

    byte lower = value & 0xFFu;
    byte upper = (value & 0xFF00u) >> 8u;
    gba_write_byte(address, lower);
    gba_write_byte(address + 1, upper);
}

word gba_read_word(word address) {
    address &= ~(sizeof(word) - 1);

    if (is_ioreg(address)) {
        byte ioreg_size = get_ioreg_size_for_addr(address);
        if(ioreg_size == sizeof(word)) {
            return read_word_ioreg(address);
        } else if (ioreg_size == 0) {
            return open_bus(address);
        } else if (!is_ioreg_readable(address)
                   && !is_ioreg_readable(address + 1)
                   && !is_ioreg_readable(address + 2)
                   && !is_ioreg_readable(address + 3)) {
            return open_bus(address);
        }
        // Otherwise, it'll be smaller than a word, and we'll read each part from the respective registers.
    }
    if (is_open_bus(address)) {
        return open_bus(address);
    }
    word lower = inline_gba_read_half(address);
    word upper = inline_gba_read_half(address + 2);

    return (upper << 16u) | lower;
}

void gba_write_word(word address, word value) {
    address &= ~(sizeof(word) - 1);
    if (is_ioreg(address)) {
        byte ioreg_size = get_ioreg_size_for_addr(address);
        if(ioreg_size == sizeof(word)) {
            write_word_ioreg(address, value);
            return;
        } else if (ioreg_size == 0) {
            logwarn("Unused word size ioregister!")
            // Unused io register
            return;
        }
        // Otherwise, it'll be smaller than a word, and we'll write each part to the respective registers.
    }

    half lower = (value & 0xFFFFu);
    half upper = (value & 0xFFFF0000u) >> 16u;

    gba_write_half(address, lower);
    gba_write_half(address + 2, upper);
}
