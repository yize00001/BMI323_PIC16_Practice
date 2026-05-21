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
#include "bmi323.h"
#include "bmm350.h"

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
static int16_t acc_pitch_offset = 0;

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

    // Map calibrated ACC pitch |0~90 deg| -> PWM duty 0~1023
    int16_t cal_pitch = acc_pitch - acc_pitch_offset;
    int16_t led_pitch = cal_pitch < 0 ? -cal_pitch : cal_pitch;
    if (led_pitch > 90) led_pitch = 90;
    PWM3_LoadDutyValue((uint16_t)((int32_t)led_pitch * 1023L / 90L));

    printf("PITCH=%4d ROLL=%4d YAW=%4d deg\r\n",
           pitch10 / 10, roll10 / 10, yaw);
}

// =========================================================
// UART RX command handler
// 'a' -> print ACC raw   'g' -> print GYRO raw
// 't' -> print TEMP      'm' -> print MAG raw
// =========================================================

static uint8_t auto_print = 1;

static void handle_uart_rx(void)
{
    int16_t ax = 0, ay = 0, az = 0;
    int16_t gx = 0, gy = 0, gz = 0;
    int32_t mx = 0, my = 0, mz = 0;

    if (!UART1__IsRxReady()) return;
    uint8_t cmd = UART1_Read();

    switch (cmd)
    {
        case 's':
            auto_print ^= 1;
            printf(auto_print ? ">> auto ON\r\n" : ">> auto OFF\r\n");
            break;
        case 'a':
            bmi323_read_axes(BMI323_ACCEL_X, &ax, &ay, &az);
            printf("ACC X=%6d Y=%6d Z=%6d\r\n", ax, ay, az);
            break;
        case 'g':
            bmi323_read_axes(BMI323_GYRO_X, &gx, &gy, &gz);
            printf("GYRO X=%6d Y=%6d Z=%6d\r\n", gx, gy, gz);
            break;
        case 't':
        {
            int16_t raw = bmi323_read_reg(BMI323_TEMP);
            int16_t t10 = (int16_t)((int32_t)raw * 10L / 512L + 230L);
            printf("TEMP=%3d.%d C\r\n", t10 / 10, t10 < 0 ? (-t10) % 10 : t10 % 10);
            break;
        }
        case 'm':
            bmm350_write_reg(BMM350_PMU_CMD, BMM350_PMU_FORCED);
            __delay_ms(20);
            bmm350_read_mag(&mx, &my, &mz);
            printf("MAG X=%7ld Y=%7ld Z=%7ld\r\n", mx, my, mz);
            break;
        default:
            printf("? s=stop/go a=ACC g=GYRO t=TEMP m=MAG\r\n");
            break;
    }
}

// =========================================================
// Timer2 — 10ms callback, 100ms sampling flag
// =========================================================

static volatile uint8_t sample_flag = 0;

static void timer0_callback(void)
{
    sample_flag = 1;  // TMR0 period = 100ms, direct flag
}

// =========================================================
// Main
// =========================================================

static void i2c_bus_recovery(void)
{
    uint8_t i;
    TRISBbits.TRISB4 = 0;   // SCL output
    TRISBbits.TRISB6 = 1;   // SDA input
    LATBbits.LATB4   = 1;

    for (i = 0; i < 9; i++) {
        LATBbits.LATB4 = 0; __delay_us(10);
        LATBbits.LATB4 = 1; __delay_us(10);
        if (PORTBbits.RB6) break;  // SDA released
    }
    // STOP condition
    TRISBbits.TRISB6 = 0;
    LATBbits.LATB6 = 0; __delay_us(10);
    LATBbits.LATB4 = 1; __delay_us(10);
    LATBbits.LATB6 = 1; __delay_us(10);
    // Release for MCC I2C
    TRISBbits.TRISB4 = 1;
    TRISBbits.TRISB6 = 1;
}

int main(void)
{
    i2c_bus_recovery();
    SYSTEM_Initialize();
    TMR0_OverflowCallbackRegister(timer0_callback);  // register before first 100ms overflow
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();
    __delay_ms(10);

    uint8_t chip_id = (uint8_t)bmi323_read_reg(0x00);
    printf("BMI323 Chip ID = 0x%02X (%s)\r\n", chip_id,
           chip_id == BMI323_CHIP_ID ? "OK" : "ERROR");
    if (chip_id != BMI323_CHIP_ID)
        while (1);

    bmi323_init();

    // Calibrate ACC pitch offset — keep board still during startup
    {
        int16_t cx = 0, cy = 0, cz = 0;
        printf("Calibrating... keep still\r\n");
        bmi323_read_axes(BMI323_ACCEL_X, &cx, &cy, &cz);
        acc_pitch_offset = iatan2(-(int32_t)cx, -(int32_t)cz);
        printf("Pitch offset = %d deg\r\n", acc_pitch_offset);
    }

    bmm350_init();

    TMR0_OverflowCallbackRegister(timer0_callback);  // already started by SYSTEM_Initialize

    while (1)
    {
        handle_uart_rx();

        if (!sample_flag) continue;
        sample_flag = 0;
        CLRWDT();
        SLEEP_IND_Toggle();   // blinks at 5Hz to confirm wake from sleep

        int16_t ax = 0, ay = 0, az = 0;
        int16_t gx = 0, gy = 0, gz = 0;
        int32_t mx = 0, my = 0, mz = 0;

        uint8_t status = (uint8_t)bmi323_read_reg(BMI323_STATUS);
        if (status & 0x80) bmi323_read_axes(BMI323_ACCEL_X, &ax, &ay, &az);
        if (status & 0x40) bmi323_read_axes(BMI323_GYRO_X,  &gx, &gy, &gz);

        __delay_ms(1);
        int16_t temp_raw = bmi323_read_reg(BMI323_TEMP);
        int16_t temp10 = (int16_t)((int32_t)temp_raw * 10L / 512L + 230L);

        bmm350_write_reg(BMM350_PMU_CMD, BMM350_PMU_FORCED);
        __delay_ms(20);
        bmm350_read_mag(&mx, &my, &mz);

        if (!auto_print) continue;

        if ((status & 0x80) && (status & 0x40))
            compute_angles(ax, ay, az, gx, gy, mx, my);

        printf("TEMP=%3d.%d C\r\n", temp10 / 10, temp10 < 0 ? (-temp10) % 10 : temp10 % 10);

        while (!TX1STAbits.TRMT);  // wait UART TX shift register empty
        SLEEP();  // wake by TMR0 overflow (100ms, LFINTOSC runs in sleep)
    }
}
