#include "single_data_swap.h"
#include "../../common/log.h"
#include "../shifts.h"

void single_data_swap(arm7tdmi_t* state, single_data_swap_t* instr) {
    unimplemented(instr->rm == 15, "Must not use PC for rm")
    unimplemented(instr->rd == 15, "Must not use PC for rd")
    unimplemented(instr->rn == 15, "Must not use PC for rn")

    logdebug("numbers: rm: %d rd: %d rn: %d", instr->rm, instr->rd, instr->rn);
    logdebug("values: rm: 0x%08X rd: 0x%08X rn: 0x%08X",
            get_register(state, instr->rm), get_register(state, instr->rd), get_register(state, instr->rn));

    word address = get_register(state, instr->rn);

    if (instr->b) {
        set_register(state, instr->rd, state->read_byte(address));
        state->write_byte(address, get_register(state, instr->rm));
    } else {
        word temp = state->read_word(address);
        temp = arm_ror(NULL, temp, (address & 3u) << 3);
        set_register(state, instr->rd, temp);
        state->write_word(address, get_register(state, instr->rm));
    }
}
