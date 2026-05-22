#ifndef X9C103S_H
#define X9C103S_H

#include <stdint.h>
#include "mcc_generated_files/system/system.h"

// Pin macros from MCC
#define X9C_CS_High()   X9C_CS_SetHigh()
#define X9C_CS_Low()    X9C_CS_SetLow()
#define X9C_INC_High()  X9C_INC_SetHigh()
#define X9C_INC_Low()   X9C_INC_SetLow()
#define X9C_UD_High()   X9C_UD_SetHigh()
#define X9C_UD_Low()    X9C_UD_SetLow()

#define X9C_MAX_POS  99
#define X9C_MIN_POS   0

void x9c103s_init(void);
void x9c103s_step_up(void);
void x9c103s_step_down(void);
void x9c103s_goto_position(uint8_t target);
uint8_t x9c103s_get_position(void);

#endif // X9C103S_H
