#include "bmi323.h"
#include "mcc_generated_files/system/system.h"

int16_t bmi323_read_reg(uint8_t reg)
{
    uint8_t buf[4] = {0};
    I2C1_WriteRead(BMI323_ADDR, &reg, 1, buf, 4);
    while (I2C1_IsBusy()) { CLRWDT(); }
    return (int16_t)((buf[3] << 8) | buf[2]);
}

void bmi323_write_reg(uint8_t reg, uint16_t value)
{
    uint8_t buf[3] = { reg, value & 0xFF, (value >> 8) & 0xFF };
    I2C1_Write(BMI323_ADDR, buf, 3);
    while (I2C1_IsBusy()) { CLRWDT(); }
}

void bmi323_read_axes(uint8_t start_reg, int16_t *v0, int16_t *v1, int16_t *v2)
{
    uint8_t buf[8] = {0};
    I2C1_WriteRead(BMI323_ADDR, &start_reg, 1, buf, 8);
    while (I2C1_IsBusy()) { CLRWDT(); }
    *v0 = (int16_t)((buf[3] << 8) | buf[2]);
    *v1 = (int16_t)((buf[5] << 8) | buf[4]);
    *v2 = (int16_t)((buf[7] << 8) | buf[6]);
}

void bmi323_init(void)
{
    bmi323_write_reg(BMI323_ACC_CONF,  0x7088);
    __delay_ms(10);
    bmi323_write_reg(BMI323_GYRO_CONF, 0x708C);
    __delay_ms(10);
    printf("BMI323 init done\r\n");
}
