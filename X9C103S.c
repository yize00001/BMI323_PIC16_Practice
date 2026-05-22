#include "X9C103S.h"

static uint8_t current_position = 0;

void x9c103s_init(void)
{
    X9C_CS_High();
    X9C_INC_High();
    X9C_UD_High();
    __delay_ms(1);

    // Synchronize to position 0: send 100 step-down pulses
    // CS stays low throughout to avoid NVM write on each pulse
    uint8_t i;
    X9C_UD_Low();
    X9C_CS_Low();
    for (i = 0; i < 100; i++) {
        X9C_INC_Low();
        __delay_us(10);
        X9C_INC_High();
        __delay_us(10);
    }
    X9C_CS_High();
    current_position = 0;
    printf("X9C103S init OK, pos=0\r\n");
}

void x9c103s_step_up(void)
{
    if (current_position >= X9C_MAX_POS) return;
    X9C_UD_High();
    X9C_CS_Low();
    X9C_INC_Low();
    __delay_us(10);
    X9C_INC_High();
    __delay_us(10);
    X9C_CS_High();
    current_position++;
}

void x9c103s_step_down(void)
{
    if (current_position <= X9C_MIN_POS) return;
    X9C_UD_Low();
    X9C_CS_Low();
    X9C_INC_Low();
    __delay_us(10);
    X9C_INC_High();
    __delay_us(10);
    X9C_CS_High();
    current_position--;
}

void x9c103s_goto_position(uint8_t target)
{
    if (target > X9C_MAX_POS) target = X9C_MAX_POS;
    while (current_position != target) {
        if (current_position < target)
            x9c103s_step_up();
        else
            x9c103s_step_down();
    }
    printf("X9C pos=%d (~%d ohm)\r\n",
           current_position,
           (int)((uint32_t)current_position * 10000U / 99U));
}

uint8_t x9c103s_get_position(void)
{
    return current_position;
}
