#ifndef BMM350_H
#define BMM350_H

#include <stdint.h>

#define BMM350_ADDR         0x14
#define BMM350_CHIP_ID      0x33
#define BMM350_PMU_CMD      0x06
#define BMM350_PMU_STATUS   0x07
#define BMM350_MAG_X_XLSB   0x31
#define BMM350_PMU_SUSPEND  0x00
#define BMM350_PMU_NORMAL   0x01
#define BMM350_PMU_FORCED   0x03

uint8_t bmm350_read_reg(uint8_t reg);
void    bmm350_write_reg(uint8_t reg, uint8_t value);
void    bmm350_read_mag(int32_t *mx, int32_t *my, int32_t *mz);
void    bmm350_init(void);

#endif // BMM350_H
