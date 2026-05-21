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

// =========================================================
// BMI323 (I2C addr 0x68)
// =========================================================
#define BMI323_ADDR       0x68
#define BMI323_CHIP_ID    0x43
#define BMI323_STATUS     0x02
#define BMI323_ACCEL_X    0x03
#define BMI323_GYRO_X     0x06
#define BMI323_ACC_CONF   0x20  // 0x7088 = 2G, 100Hz, high-perf
#define BMI323_GYRO_CONF  0x21  // 0x708C = 2000dps, 100Hz, high-perf

// I2C protocol: each read prepends 2 dummy bytes
// Single reg: WriteRead(reg, 4) -> buf[2]=LSB, buf[3]=MSB
// Burst 3-axis: WriteRead(reg, 8) -> buf[2..7] = X_L,X_H,Y_L,Y_H,Z_L,Z_H

static int16_t bmi323_read_reg(uint8_t reg)
{
    uint8_t buf[4] = {0};
    I2C1_WriteRead(BMI323_ADDR, &reg, 1, buf, 4);
    while (I2C1_IsBusy()) { CLRWDT(); }
    return (int16_t)((buf[3] << 8) | buf[2]);
}

static void bmi323_write_reg(uint8_t reg, uint16_t value)
{
    uint8_t buf[3] = { reg, value & 0xFF, (value >> 8) & 0xFF };
    I2C1_Write(BMI323_ADDR, buf, 3);
    while (I2C1_IsBusy()) { CLRWDT(); }
}

static void bmi323_read_axes(uint8_t start_reg, int16_t *v0, int16_t *v1, int16_t *v2)
{
    uint8_t buf[8] = {0};
    I2C1_WriteRead(BMI323_ADDR, &start_reg, 1, buf, 8);
    while (I2C1_IsBusy()) { CLRWDT(); }
    *v0 = (int16_t)((buf[3] << 8) | buf[2]);
    *v1 = (int16_t)((buf[5] << 8) | buf[4]);
    *v2 = (int16_t)((buf[7] << 8) | buf[6]);
}

static void bmi323_init(void)
{
    bmi323_write_reg(BMI323_ACC_CONF,  0x7088);
    __delay_ms(10);
    bmi323_write_reg(BMI323_GYRO_CONF, 0x708C);
    __delay_ms(10);
    printf("BMI323 init done\r\n");
}

// =========================================================
// BMM350 (I2C addr 0x14)
// =========================================================
#define BMM350_ADDR             0x14
#define BMM350_CHIP_ID          0x33
#define BMM350_PMU_CMD          0x06
#define BMM350_PMU_STATUS       0x07
#define BMM350_MAG_X_XLSB       0x31
#define BMM350_PMU_NORMAL       0x01
#define BMM350_PMU_FORCED       0x02

// I2C protocol: same 2-dummy-byte rule as BMI323
// Single reg: WriteRead(reg, 3) -> buf[2] = data
// MAG burst: WriteRead(0x31, 11) -> buf[2..10] = X(3B), Y(3B), Z(3B), each 24-bit signed

static uint8_t bmm350_read_reg(uint8_t reg)
{
    uint8_t buf[3] = {0};
    I2C1_WriteRead(BMM350_ADDR, &reg, 1, buf, 3);
    while (I2C1_IsBusy()) { CLRWDT(); }
    return buf[2];
}

static void bmm350_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };
    I2C1_Write(BMM350_ADDR, buf, 2);
    while (I2C1_IsBusy()) { CLRWDT(); }
}

static void bmm350_read_mag(int32_t *mx, int32_t *my, int32_t *mz)
{
    uint8_t buf[11] = {0};
    uint8_t reg = BMM350_MAG_X_XLSB;
    I2C1_WriteRead(BMM350_ADDR, &reg, 1, buf, 11);
    while (I2C1_IsBusy()) { CLRWDT(); }

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
    uint8_t pmu;
    uint8_t i;

    uint8_t id = bmm350_read_reg(0x00);
    printf("BMM350 Chip ID = 0x%02X (%s)\r\n", id,
           id == BMM350_CHIP_ID ? "OK" : "ERROR");
    if (id != BMM350_CHIP_ID)
        while (1);

    // Wait for power-on OTP loading (~500ms, use 2s margin)
    // Avoid I2C calls during this window to prevent stack overflow
    for (i = 0; i < 40; i++) __delay_ms(50);

    pmu = bmm350_read_reg(BMM350_PMU_STATUS);
    printf("BMM350 OTP PMU=0x%02X\r\n", pmu);

    // Required unlock sequence before mode change (from GPA7740 reference driver)
    bmm350_write_reg(0x50, 0x80);
    __delay_ms(10);

    bmm350_write_reg(BMM350_PMU_CMD, BMM350_PMU_NORMAL);
    for (i = 0; i < 4; i++) __delay_ms(50);  // 200ms for Normal mode transition

    pmu = bmm350_read_reg(BMM350_PMU_STATUS);
    printf("BMM350 PMU = 0x%02X (%s)\r\n", pmu,
           (pmu & 0x08) ? "normal" : "not normal");
    printf("BMM350 init done\r\n");
}

// =========================================================
// Complementary Filter  (integer-only, no math.h)
//
// Angles in degrees*10: pitch10=456 means 45.6 deg
// ACC scale:  2G range -> 1g = 16384 LSB
// GYRO scale: 2000dps / 32768 = 0.061035 deg/LSB
//             per 100ms step: LSB * 0.0061035 deg = LSB * 61 / 10000 deg*10
// Sensor mounted inverted: az < 0 when flat -> negate az for atan2
// =========================================================

static int16_t pitch10 = 0, roll10 = 0;

// Integer atan2 approximation, returns -180 to +180 degrees, error < 3 deg
static int16_t iatan2(int32_t y, int32_t x)
{
    int32_t ax = x < 0 ? -x : x;
    int32_t ay = y < 0 ? -y : y;
    int16_t r;
    if (ax == 0 && ay == 0) return 0;
    if (ax >= ay)
        r = (int16_t)((ay * 45L) / ax);
    else
        r = (int16_t)(90L - (ax * 45L) / ay);
    if      (x >= 0 && y >= 0) return  r;
    else if (x <  0 && y >= 0) return  180 - r;
    else if (x <  0 && y <  0) return -(180 - r);
    else                        return -r;
}

static void compute_angles(int16_t ax, int16_t ay, int16_t az,
                            int16_t gx, int16_t gy,
                            int32_t mx, int32_t my)
{
    int16_t acc_pitch = iatan2(-(int32_t)ax, -(int32_t)az);
    int16_t acc_roll  = iatan2( (int32_t)ay, -(int32_t)az);

    int16_t dpitch = (int16_t)((int32_t)gy * 61L / 10000L);
    int16_t droll  = (int16_t)((int32_t)gx * 61L / 10000L);

    // 98% gyro integration + 2% ACC correction
    pitch10 = (int16_t)((int32_t)(pitch10 + dpitch) * 98L / 100L
                       + (int32_t)acc_pitch * 20L / 100L);
    roll10  = (int16_t)((int32_t)(roll10  + droll)  * 98L / 100L
                       + (int32_t)acc_roll  * 20L / 100L);

    // 2D yaw from MAG X/Y (>> 8 to prevent int32 overflow in iatan2)
    int16_t yaw = iatan2(-(int32_t)(my >> 8), (int32_t)(mx >> 8));
    if (yaw < 0) yaw += 360;

    printf("PITCH=%4d ROLL=%4d YAW=%4d deg\r\n",
           pitch10 / 10, roll10 / 10, yaw);
}

// =========================================================
// Main
// =========================================================

int main(void)
{
    SYSTEM_Initialize();
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();
    __delay_ms(10);

    uint8_t chip_id = (uint8_t)bmi323_read_reg(0x00);
    printf("BMI323 Chip ID = 0x%02X (%s)\r\n", chip_id,
           chip_id == BMI323_CHIP_ID ? "OK" : "ERROR");
    if (chip_id != BMI323_CHIP_ID)
        while (1);

    bmi323_init();
    bmm350_init();

    while (1)
    {
        CLRWDT();

        int16_t ax = 0, ay = 0, az = 0;
        int16_t gx = 0, gy = 0, gz = 0;
        int32_t mx = 0, my = 0, mz = 0;

        uint8_t status = (uint8_t)bmi323_read_reg(BMI323_STATUS);
        if (status & 0x80) bmi323_read_axes(BMI323_ACCEL_X, &ax, &ay, &az);
        if (status & 0x40) bmi323_read_axes(BMI323_GYRO_X,  &gx, &gy, &gz);

        bmm350_write_reg(BMM350_PMU_CMD, BMM350_PMU_FORCED);
        __delay_ms(20);
        bmm350_read_mag(&mx, &my, &mz);

        if ((status & 0x80) && (status & 0x40))
            compute_angles(ax, ay, az, gx, gy, mx, my);

        __delay_ms(100);
    }
}
