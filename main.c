 /*
 * MAIN Generated Driver File
 *
 * @file main.c
 *
 * @defgroup main MAIN
 *
 * @brief This is the generated driver implementation file for the MAIN driver.
 *
 * @version MAIN Driver Version 1.0.2
 *
 * @version Package Version: 3.1.2
*/

/*
? [2026] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip
    software and any derivatives exclusively with Microchip products.
    You are responsible for complying with 3rd party license terms
    applicable to your use of 3rd party software (including open source
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.?
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR
    THIS SOFTWARE.
*/
#include "mcc_generated_files/system/system.h"

// BMI323
#define BMI323_ADDR      0x68
#define BMI323_CHIP_ID   0x43
#define BMI323_ACC_CONF  0x20
#define BMI323_GYRO_CONF 0x21
#define BMI323_STATUS    0x02
#define BMI323_ACCEL_X   0x03
#define BMI323_GYRO_X    0x06

// BMM350
#define BMM350_ADDR        0x14
#define BMM350_CHIP_ID     0x33
#define BMM350_PMU_CMD     0x06
#define BMM350_PMU_STATUS  0x07
#define BMM350_MAG_X_XLSB  0x31

// ---- BMI323 ----

// I2C read: 2 dummy bytes then 2 data bytes (LSB first)
static int16_t bmi323_read_reg(uint8_t reg)
{
    uint8_t buf[4] = {0};
    I2C1_WriteRead(BMI323_ADDR, &reg, 1, buf, 4);
    while(I2C1_IsBusy()) { CLRWDT(); }
    return (int16_t)((buf[3] << 8) | buf[2]);
}

// Write 16-bit value: [reg, LSB, MSB]
static void bmi323_write_reg(uint8_t reg, uint16_t value)
{
    uint8_t buf[3] = { reg, value & 0xFF, (value >> 8) & 0xFF };
    I2C1_Write(BMI323_ADDR, buf, 3);
    while(I2C1_IsBusy()) { CLRWDT(); }
}

// Burst read 3 axes: 2 dummy + 6 data = 8 bytes
static void bmi323_read_axes(uint8_t start_reg, int16_t *v0, int16_t *v1, int16_t *v2)
{
    uint8_t buf[8] = {0};
    I2C1_WriteRead(BMI323_ADDR, &start_reg, 1, buf, 8);
    while(I2C1_IsBusy()) { CLRWDT(); }
    *v0 = (int16_t)((buf[3] << 8) | buf[2]);
    *v1 = (int16_t)((buf[5] << 8) | buf[4]);
    *v2 = (int16_t)((buf[7] << 8) | buf[6]);
}

static void bmi323_init(void)
{
    bmi323_write_reg(BMI323_ACC_CONF,  0x7088);  // 2G, 100Hz, high perf
    __delay_ms(10);
    bmi323_write_reg(BMI323_GYRO_CONF, 0x708C);  // 2000dps, 100Hz, high perf
    __delay_ms(10);
    printf("BMI323 init done\r\n");
}

// ---- BMM350 ----

// I2C read: 2 dummy bytes then 1 data byte
static uint8_t bmm350_read_reg(uint8_t reg)
{
    uint8_t buf[3] = {0};
    I2C1_WriteRead(BMM350_ADDR, &reg, 1, buf, 3);
    while(I2C1_IsBusy()) { CLRWDT(); }
    return buf[2];
}

// Write 8-bit value: [reg, data]
static void bmm350_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };
    I2C1_Write(BMM350_ADDR, buf, 2);
    while(I2C1_IsBusy()) { CLRWDT(); }
}

// Burst read 3 axes: 2 dummy + 9 data = 11 bytes, each axis is 24-bit (XLSB/LSB/MSB)
static void bmm350_read_mag(int32_t *mx, int32_t *my, int32_t *mz)
{
    uint8_t buf[11] = {0};
    uint8_t reg = BMM350_MAG_X_XLSB;
    I2C1_WriteRead(BMM350_ADDR, &reg, 1, buf, 11);
    while(I2C1_IsBusy()) { CLRWDT(); }

    int32_t x = ((int32_t)buf[4] << 16) | ((int32_t)buf[3] << 8) | buf[2];
    int32_t y = ((int32_t)buf[7] << 16) | ((int32_t)buf[6] << 8) | buf[5];
    int32_t z = ((int32_t)buf[10] << 16) | ((int32_t)buf[9] << 8) | buf[8];

    // Sign extend 24-bit -> 32-bit
    if (x & 0x800000L) x |= (int32_t)0xFF000000L;
    if (y & 0x800000L) y |= (int32_t)0xFF000000L;
    if (z & 0x800000L) z |= (int32_t)0xFF000000L;

    *mx = x;
    *my = y;
    *mz = z;
}

static void bmm350_init(void)
{
    uint8_t id = bmm350_read_reg(0x00);
    printf("BMM350 Chip ID = 0x%02X (%s)\r\n", id,
           id == BMM350_CHIP_ID ? "OK" : "ERROR");
    if (id != BMM350_CHIP_ID)
        while(1);

    // Wait for power-on OTP loading (no I2C calls to avoid stack overflow)
    // OTP loading completes within ~500ms; use 2s to be safe
    uint8_t i;
    for (i = 0; i < 40; i++) __delay_ms(50);

    uint8_t pmu = bmm350_read_reg(BMM350_PMU_STATUS);
    printf("BMM350 OTP PMU=0x%02X\r\n", pmu);

    // GPA7 driver: write 0x50=0x80 before NM
    bmm350_write_reg(0x50, 0x80);
    __delay_ms(10);

    // Normal mode
    bmm350_write_reg(BMM350_PMU_CMD, 0x01);
    for (i = 0; i < 4; i++) __delay_ms(50);  // 200ms

    pmu = bmm350_read_reg(BMM350_PMU_STATUS);
    printf("BMM350 PMU = 0x%02X (%s)\r\n", pmu,
           (pmu & 0x08) ? "normal" : "not normal");
    printf("BMM350 init done\r\n");
}

int main(void)
{
    SYSTEM_Initialize();
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();
    __delay_ms(10);

    uint8_t chip_id = (uint8_t)bmi323_read_reg(0x00);
    printf("BMI323 Chip ID = 0x%02X (%s)\r\n", chip_id,
           chip_id == BMI323_CHIP_ID ? "OK" : "ERROR");
    if(chip_id != BMI323_CHIP_ID)
        while(1);

    bmi323_init();
    bmm350_init();

    while(1)
    {
        CLRWDT();
        uint8_t status = (uint8_t)bmi323_read_reg(BMI323_STATUS);
        int16_t ax, ay, az, gx, gy, gz;
        int32_t mx, my, mz;

        if(status & 0x80)
            bmi323_read_axes(BMI323_ACCEL_X, &ax, &ay, &az);

        if(status & 0x40)
            bmi323_read_axes(BMI323_GYRO_X, &gx, &gy, &gz);

        // Forced mode: trigger one-shot measurement then read
        bmm350_write_reg(BMM350_PMU_CMD, 0x02);
        __delay_ms(20);
        bmm350_read_mag(&mx, &my, &mz);

        if(status & 0x80)
            printf("ACC  X=%6d Y=%6d Z=%6d\r\n", ax, ay, az);

        if(status & 0x40)
            printf("GYRO X=%6d Y=%6d Z=%6d\r\n", gx, gy, gz);

        printf("MAG  X=%8ld Y=%8ld Z=%8ld\r\n", mx, my, mz);

        __delay_ms(100);
    }
}
