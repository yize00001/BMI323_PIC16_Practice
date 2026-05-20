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

#define BMI323_ADDR      0x68
#define BMI323_CHIP_ID   0x43
#define BMI323_ACC_CONF  0x20
#define BMI323_GYRO_CONF 0x21
#define BMI323_STATUS    0x02
#define BMI323_ACCEL_X   0x03
#define BMI323_GYRO_X    0x06

// Single register read: returns 16-bit value (2 dummy + 2 data bytes)
static int16_t bmi323_read_reg(uint8_t reg)
{
    uint8_t buf[4] = {0};
    I2C1_WriteRead(BMI323_ADDR, &reg, 1, buf, 4);
    while(I2C1_IsBusy());
    return (int16_t)((buf[3] << 8) | buf[2]);
}

// Write 16-bit value to register (reg + LSB + MSB)
static void bmi323_write_reg(uint8_t reg, uint16_t value)
{
    uint8_t buf[3] = { reg, value & 0xFF, (value >> 8) & 0xFF };
    I2C1_Write(BMI323_ADDR, buf, 3);
    while(I2C1_IsBusy());
}

// Burst read 3 axes: 2 dummy + 6 data bytes = 8 bytes total
static void bmi323_read_axes(uint8_t start_reg, int16_t *v0, int16_t *v1, int16_t *v2)
{
    uint8_t buf[8] = {0};
    I2C1_WriteRead(BMI323_ADDR, &start_reg, 1, buf, 8);
    while(I2C1_IsBusy());
    *v0 = (int16_t)((buf[3] << 8) | buf[2]);
    *v1 = (int16_t)((buf[5] << 8) | buf[4]);
    *v2 = (int16_t)((buf[7] << 8) | buf[6]);
}

static void bmi323_init(void)
{
    bmi323_write_reg(BMI323_ACC_CONF, 0x7088);   // 2G, 100Hz, high perf
    __delay_ms(10);
    bmi323_write_reg(BMI323_GYRO_CONF, 0x708C);  // 2000dps, 100Hz, high perf
    __delay_ms(10);
    printf("BMI323 init done\r\n");
}

int main(void)
{
    SYSTEM_Initialize();
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();
    __delay_ms(10);

    uint8_t chip_id = (uint8_t)bmi323_read_reg(0x00);
    printf("Chip ID = 0x%02X (%s)\r\n", chip_id,
           chip_id == BMI323_CHIP_ID ? "OK" : "ERROR");

    if(chip_id != BMI323_CHIP_ID)
        while(1);

    bmi323_init();

    while(1)
    {
        uint8_t status = (uint8_t)bmi323_read_reg(BMI323_STATUS);
        int16_t ax, ay, az, gx, gy, gz;

        if(status & 0x80)
            bmi323_read_axes(BMI323_ACCEL_X, &ax, &ay, &az);

        if(status & 0x40)
            bmi323_read_axes(BMI323_GYRO_X, &gx, &gy, &gz);

        if(status & 0x80)
            printf("ACC  X=%6d Y=%6d Z=%6d\r\n", ax, ay, az);

        if(status & 0x40)
            printf("GYRO X=%6d Y=%6d Z=%6d\r\n", gx, gy, gz);

        __delay_ms(100);
    }
}
