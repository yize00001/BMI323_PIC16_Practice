#include "bmm350.h"
#include "mcc_generated_files/system/system.h"

uint8_t bmm350_read_reg(uint8_t reg)
{
    uint8_t buf[3] = {0};
    I2C1_WriteRead(BMM350_ADDR, &reg, 1, buf, 3);
    while (I2C1_IsBusy()) { CLRWDT(); }
    return buf[2];
}

void bmm350_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };
    I2C1_Write(BMM350_ADDR, buf, 2);
    while (I2C1_IsBusy()) { CLRWDT(); }
}

void bmm350_read_mag(int32_t *mx, int32_t *my, int32_t *mz)
{
    uint8_t buf[11] = {0};
    uint8_t reg = BMM350_MAG_X_XLSB;
    I2C1_WriteRead(BMM350_ADDR, &reg, 1, buf, 11);
    while (I2C1_IsBusy()) { CLRWDT(); }

    int32_t x = ((int32_t)buf[4] << 16) | ((int32_t)buf[3] << 8) | buf[2];
    int32_t y = ((int32_t)buf[7] << 16) | ((int32_t)buf[6] << 8) | buf[5];
    int32_t z = ((int32_t)buf[10] << 16) | ((int32_t)buf[9] << 8) | buf[8];

    if (x & 0x800000L) x |= (int32_t)0xFF000000L;
    if (y & 0x800000L) y |= (int32_t)0xFF000000L;
    if (z & 0x800000L) z |= (int32_t)0xFF000000L;

    *mx = x;
    *my = y;
    *mz = z;
}

void bmm350_init(void)
{
    uint8_t pmu;
    uint8_t i;

    // Soft reset: clears cmd_illegal and all error state
    bmm350_write_reg(0x7E, 0xB6);
    __delay_ms(5);

    uint8_t id = bmm350_read_reg(0x00);
    printf("BMM350 Chip ID = 0x%02X (%s)\r\n", id,
           id == BMM350_CHIP_ID ? "OK" : "ERROR");
    if (id != BMM350_CHIP_ID)
        while (1);

    for (i = 0; i < 40; i++) __delay_ms(50);

    pmu = bmm350_read_reg(BMM350_PMU_STATUS);
    printf("BMM350 OTP PMU=0x%02X\r\n", pmu);

    bmm350_write_reg(0x50, 0x80);
    __delay_ms(10);

    bmm350_write_reg(BMM350_PMU_CMD, BMM350_PMU_NORMAL);
    for (i = 0; i < 4; i++) __delay_ms(50);

    pmu = bmm350_read_reg(BMM350_PMU_STATUS);
    printf("BMM350 PMU = 0x%02X (%s)\r\n", pmu,
           (pmu & 0x08) ? "normal" : "not normal");
    printf("BMM350 init done\r\n");
}
