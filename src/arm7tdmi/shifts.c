#include "../common/log.h"
#include "arm7tdmi.h"
#include "shifts.h"

word arm_lsr(status_register_t* cpsr, word data, word shift_amount) {
    logdebug("LSR shift")
    word result;

    if (shift_amount == 0) {
        return data;
    }

    if (shift_amount < 32) {
        result = data >> shift_amount;
        if (cpsr) {
            cpsr->C = (data >> (shift_amount - 1u)) & 1u;
        }

    } else {
        result = 0;
        if (cpsr) {
            // This has to be a special case since C doesn't like it when you >> by >= the width of a type
            if (shift_amount == 32) {
                cpsr->C = data >> 31u;
            } else {
                cpsr->C = 0;
            }
        }
    }
    return result;
}

word arm_lsl(status_register_t* cpsr, word data, word shift_amount) {
    logdebug("LSL shift")
    if (shift_amount == 0) {
        return data;
    } else {
        word result;
        if (shift_amount < 32) {
            result = data << shift_amount;
            if (cpsr) {
                cpsr->C = (data << (shift_amount - 1u)) >> 31u;
            }
        }
        else {
            result = 0;
            if (cpsr) {
                // This has to be a special case since C doesn't like it when you << by >= the width of a type
                if (shift_amount == 32) {
                    cpsr->C = data & 1u;
                } else {
                    cpsr->C = 0;
                }
            }
        }
        logdebug("result: 0x%08X", result)
        return result;
    }

}

word arm_asr(status_register_t* cpsr, word data, word shift_amount) {
    word result;
    if (shift_amount == 0) {
        result = data;
    } else {
        if (shift_amount < 32) {
            if (cpsr) {
                cpsr->C = (data >> (shift_amount - 1u)) & 1u;
            }
            int32_t signed_data = data;
            signed_data >>= shift_amount;
            result = signed_data;
        }
        else {
            int32_t signed_data = data;
            signed_data >>= 31u;
            result = signed_data;

            if (cpsr) {
                cpsr->C = result & 1u;
            }
        }
    }

    return result;
}

word arm_ror(status_register_t* cpsr, word data, word shift_amount) {
    if (shift_amount == 0) {
        return data;
    }

    shift_amount &= 31u;
    word result = (data >> shift_amount) | (data << (-shift_amount & 31u));
    if (cpsr) cpsr->C = result >> 31u;
    return result;
}

word arm_shift(status_register_t* cpsr, shift_type_t type, word data, word shift_amount) {
    switch (type) {
        case LSL:
            return arm_lsl(cpsr, data, shift_amount);
        case LSR:
            return arm_lsr(cpsr, data, shift_amount);
        case ASR:
            return arm_asr(cpsr, data, shift_amount);
        case ROR:
            return arm_ror(cpsr, data, shift_amount);
        default:
            logfatal("Unknown shift type: %d", type)
    }
}
word arm_shift_special_zero_behavior(arm7tdmi_t* state, status_register_t* cpsr, shift_type_t type, word data) {
    logdebug("----handling special case, immediate shift amount 0----")
    switch (type) {
        case LSL:
            return data; // Not affected.
        case LSR:
            // Treat it as LSR#32
            return arm_shift(cpsr, LSR, data, 32);
        case ASR:
            // Treat it as ASR#32
            return arm_shift(cpsr, ASR, data, 32);
        case ROR: {
            word oldc = state->cpsr.C;
            if (cpsr) {
                cpsr->C = data & 1u;
            }
            return (oldc << 31u) | (data >> 1u);
        }
        default:
            logfatal("Unknown shift type: %d (this should never happen)", type)
    }
}
