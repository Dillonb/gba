#include <stdlib.h>
#include <stdbool.h>
#include "arm7tdmi.h"
#include "log.h"
#include "arm_instr.h"

arm7tdmi_t* init_arm7tdmi(byte (*read_byte)(uint32_t),
                          uint16_t (*read16)(uint32_t),
                          void (*write_byte)(uint32_t, byte),
                          void (*write16)(uint32_t, uint16_t)) {
    arm7tdmi_t* mem = malloc(sizeof(arm7tdmi_t));
    mem->pc = 0x08000000;
    mem->read_byte = read_byte;
    mem->read16 = read16;
    mem->write_byte = write_byte;
    mem->write16 = write16;

    return mem;
}

uint32_t read32(arm7tdmi_t* state, uint32_t addr) {
    uint32_t lower = state->read16(addr);
    uint32_t upper = state->read16(addr + 2);

    return (upper << 16u) | lower;
}

arminstr_t read32_instr(arm7tdmi_t* state, uint32_t addr) {
    arminstr_t instr;
    instr.raw = read32(state, addr);
    return instr;
}

bool check_cond(arminstr_t* instr) {
    switch (instr->parsed.cond) {
        case AL:
            return true;
        default:
            logfatal("Unimplemented COND: %d", instr->parsed.cond);
    }
}

int this_step_ticks = 0;

void tick(int ticks) {
    this_step_ticks += ticks;
}

int arm7tdmi_step(arm7tdmi_t* state) {
    this_step_ticks = 0;
    arminstr_t instr = read32_instr(state, state->pc);
    logdebug("read: 0x%04X", instr.raw)
    logdebug("cond: %d", instr.parsed.cond)
    if (check_cond(&instr)) {
        arm_instr_type_t type = get_instr_type(&instr);
        switch (type) {
            case DPFSR:
                logfatal("Unimplemented instruction type: DPFSR")
            case MULTIPLY:
                logfatal("Unimplemented instruction type: MULTIPLY")
            case MULTIPLY_LONG:
                logfatal("Unimplemented instruction type: MULTIPLY_LONG")
            case SINGLE_DATA_SWAP:
                logfatal("Unimplemented instruction type: SINGLE_DATA_SWAP")
            case BRANCH_EXCHANGE:
                logfatal("Unimplemented instruction type: BRANCH_EXCHANGE")
            case HALFWORD_DT_RO:
                logfatal("Unimplemented instruction type: HALFWORD_DT_RO")
            case HALFWORD_DT_IO:
                logfatal("Unimplemented instruction type: HALFWORD_DT_IO")
            case SINGLE_DATA_TRANSFER:
                logfatal("Unimplemented instruction type: SINGLE_DATA_TRANSFER")
            case UNDEFINED:
                logfatal("Unimplemented instruction type: UNDEFINED")
            case BLOCK_DATA_TRANSFER:
                logfatal("Unimplemented instruction type: BLOCK_DATA_TRANSFER")
            case BRANCH:
                logfatal("Unimplemented instruction type: BRANCH")
            case COPROCESSOR_DATA_TRANSFER:
                logfatal("Unimplemented instruction type: COPROCESSOR_DATA_TRANSFER")
            case COPROCESSOR_DATA_OPERATION:
                logfatal("Unimplemented instruction type: COPROCESSOR_DATA_OPERATION")
            case COPROCESSOR_REGISTER_TRANSFER:
                logfatal("Unimplemented instruction type: COPROCESSOR_REGISTER_TRANSFER")
            case SOFTWARE_INTERRUPT:
                logfatal("Unimplemented instruction type: SOFTWARE_INTERRUPT")
        }
    }
    else { // Cond told us not to execute this instruction
        tick(1);
    }
    return this_step_ticks;
}
