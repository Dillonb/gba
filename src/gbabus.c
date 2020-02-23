#include <err.h>
#include "gbabus.h"
#include "io_register_sizes.h"
#include "gbamem.h"
#include "log.h"

gbamem_t* mem;

void init_gbabus(gbamem_t* new_mem) {
    mem = new_mem;
}

void write_io_register(word addr, word value) {
    // 0x04XX0800 is the only address that's mirrored.
    if ((addr & 0xFF00FFFFu) == 0x04000800u) {
        addr = 0xFF00FFFFu;
    }

    logwarn("Wrote 0x%08X to 0x%08X", value, addr)
    unimplemented(1, "io register write")
}

byte gba_read_byte(word addr) {
    if (addr < 0x08000000) {
        logwarn("Tried to read from 0x%08X", addr)
        unimplemented(1, "Read from non-cartridge address")
    } else if (addr < 0x0E00FFFF) {
        // Cartridge
        word adjusted = addr - 0x08000000;
        if (adjusted > mem->rom_size) {
            logfatal("Attempted out of range read");
        } else {
            return mem->rom[adjusted];
        }
    }

    logfatal("Something's up, we reached the end of gba_read_byte without getting a value! addr: 0x%08X", addr)
}

half gba_read_half(word addr) {
    byte lower = gba_read_byte(addr);
    byte upper = gba_read_byte(addr + 1);

    return (upper << 8u) | lower;
}

void gba_write_byte(word addr, byte value) {
    if (addr < 0x04000000) {
        logwarn("Tried to write to 0x%08X", addr)
        unimplemented(1, "Tried to write general internal memory")
    } else if (addr < 0x04000400) {
        write_io_register(addr, value);
    } else if (addr < 0x05000000) {
        logwarn("Tried to write to 0x%08X", addr)
        unimplemented(1, "Tried to write to unused portion of general internal memory")
    } else if (addr < 0x08000000) {
        logwarn("Tried to write to 0x%08X", addr)
        unimplemented(1, "Write to internal display memory address")
    } else if (addr < 0x0E00FFFF) {
        logwarn("Tried to write to 0x%08X", addr)
        unimplemented(1, "Write to cartridge address")
    } else {
        logfatal("Something's up, we reached the end of gba_write_byte without writing a value! addr: 0x%08X", addr)
    }
}

void gba_write_half(word address, half value) {
    if (is_ioreg(address)) {
        byte ioreg_size = get_ioreg_size_for_addr(address);
        if (ioreg_size == sizeof(half)) {
            logfatal("Writing a half to half-size ioreg")
        } else if (ioreg_size == 0) {
            // Unused io register
            logwarn("Unused half size ioregister!")
            return;
        }
    }

    byte lower = value & 0xFFu;
    byte upper = (value & 0xFF00u) >> 8u;
    gba_write_byte(address, lower);
    gba_write_byte(address + 1, upper);
}

word gba_read_word(word addr) {
    word lower = gba_read_half(addr);
    word upper = gba_read_half(addr + 2);

    return (upper << 16u) | lower;
}

void gba_write_word(word address, word value) {
    if (is_ioreg(address)) {
        byte ioreg_size = get_ioreg_size_for_addr(address);
        if(ioreg_size == sizeof(word)) {
            logfatal("Writing a word to word-size ioreg")
        } else if (ioreg_size == 0) {
            logwarn("Unused word size ioregister!")
            // Unused io register
            return;
        }
    }

    half lower = (value & 0xFFFFu);
    half upper = (value & 0xFFFF0000u);

    gba_write_half(address, lower);
    gba_write_half(address + 2, upper);
}

