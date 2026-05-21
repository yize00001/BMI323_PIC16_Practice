#ifndef BMI323_H
#define BMI323_H

#include <stdint.h>

#define BMI323_ADDR       0x68
#define BMI323_CHIP_ID    0x43
#define BMI323_STATUS     0x02
#define BMI323_ACCEL_X    0x03
#define BMI323_GYRO_X     0x06
#define BMI323_TEMP       0x09
#define BMI323_ACC_CONF   0x20
#define BMI323_GYRO_CONF  0x21

int16_t bmi323_read_reg(uint8_t reg);
void    bmi323_write_reg(uint8_t reg, uint16_t value);
void    bmi323_read_axes(uint8_t start_reg, int16_t *v0, int16_t *v1, int16_t *v2);
void    bmi323_init(void);

#endif // BMI323_H
