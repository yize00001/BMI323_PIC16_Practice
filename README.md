# BMI323 + BMM350 + X9C103S on PIC16F15245

A practice project porting BMI323 IMU, BMM350 magnetometer, and X9C103S digital potentiometer drivers to PIC16F15245, featuring a complementary filter for pitch/roll/yaw angle estimation and UART-controlled resistance adjustment.

## Hardware

| Component | Description |
|---|---|
| PIC16F15245 (SSOP20) | Main MCU |
| BMI323 Breakout Board | 6-axis IMU (accelerometer + gyroscope) |
| BMM350 Breakout Board | 3-axis magnetometer |
| X9C103S Breakout Board | 10 kΩ digital potentiometer, 100 positions |
| PICkit 5 | Programmer/debugger |
| CH340 USB-UART | Serial monitor |

## Wiring

### PIC16F15245 Pinout

```
        ┌──┬──┐
  VDD ──┤1 │20├── VSS
  RA5 ──┤2 │19├── RA0 → PICkit Pin4 (ICSPDAT)
  RA4 ──┤3 │18├── RA1 → PICkit Pin5 (ICSPCLK)
 MCLR ──┤4 │17├── RA2
  RC5 ──┤5 │16├── RC0
  RC4 ──┤6 │15├── RC1
  RC3 ──┤7 │14├── RC2
  RC6 ──┤8 │13├── RB4 → SCL
  RC7 ──┤9 │12├── RB5 → CH340 TX (RX1)
  RB7 ──┤10│11├── RB6 → SDA
        └──┴──┘
```

### Connections

| Signal | PIC16 Pin | Connected To |
|---|---|---|
| SCL | RB4 (Pin 13) | BMI323 SCK, BMM350 SCL |
| SDA | RB6 (Pin 11) | BMI323 SDI, BMM350 SDA |
| TX1 | RC5 (Pin 5) | CH340 RX |
| RX1 | RB5 (Pin 12) | CH340 TX |
| PWM LED | RC3 (Pin 7) | LED + 330Ω → GND |
| Sample LED | RB7 (Pin 10) | LED + 330Ω → GND |
| Button | RA4 (Pin 3) | Button → GND (WPU enabled) |
| MCLR | RA3 (Pin 4) | PICkit Pin 1 |
| X9C_CS | RC4 (Pin 6) | X9C103S /CS |
| X9C_INC | RC6 (Pin 8) | X9C103S INC |
| X9C_UD | RC7 (Pin 9) | X9C103S U/D |

**BMI323:** CS → VDD (I2C mode), SDO → GND (address 0x68)  
**BMM350:** Fixed I2C address 0x14  
**X9C103S:** VH → 3.3 V, VL → GND, VW = wiper output

## MCC Configuration

| Peripheral | Setting |
|---|---|
| Clock | HFINTOSC 32 MHz |
| I2C (MSSP1) | Host, 100 kHz, SCL=RB4, SDA=RB6 |
| UART (EUSART1) | 115200 baud, TX=RC5, RX=RB5 |
| TMR0 | LFINTOSC 31 kHz, 16-bit, 100 ms overflow (sampling flag) |
| TMR2 | PWM base clock |
| PWM3 | RC3, duty 0–1023 |
| IOC | RA4 falling edge (button) |
| GPIO Output | RC4 (X9C_CS), RC6 (X9C_INC), RC7 (X9C_UD) |

## Features

- Read BMI323 accelerometer, gyroscope, and temperature
- Read BMM350 magnetometer
- Complementary filter: pitch / roll (gyro + ACC) and yaw (magnetometer)
- PWM LED brightness controlled by pitch angle
- 100 ms sampling period via TMR0 overflow flag
- Sample indicator: RB7 LED blinks at 5 Hz
- I2C bus recovery on startup (9 SCL pulses)
- GYRO zero-offset calibration at startup
- MAG hard-iron calibration (rotate board to collect min/max)
- X9C103S digital potentiometer: 100 positions (0–99), UART-controlled

## UART Commands

| Command | Action |
|---|---|
| `s` | Toggle auto-print on/off |
| `a` | Print raw accelerometer (X/Y/Z) |
| `g` | Print raw gyroscope (X/Y/Z) |
| `t` | Print temperature |
| `m` | Print magnetometer (X/Y/Z) |
| `r` | Re-run GYRO zero-offset calibration |
| `c` | Toggle MAG hard-iron calibration (start/stop) |
| `+` | X9C103S step up one position |
| `-` | X9C103S step down one position |
| `p##` | X9C103S go to position 00–99 (send digits immediately after `p`, no Enter) |

## Building

1. Open `BMI323_PIC16_Practice.X` in **MPLAB X IDE v6.x**
2. Toolchain: **XC8 v3.x** (free tier)
3. Build → Make and Program Device

## Project Structure

```
BMI323_PIC16_Practice.X/
├── main.c              # Main application
├── bmi323.c / .h       # BMI323 I2C driver
├── bmm350.c / .h       # BMM350 I2C driver
├── X9C103S.c / .h      # X9C103S digital potentiometer driver
└── mcc_generated_files/
    ├── system/         # Clock, pins, interrupt
    ├── i2c_host/       # MSSP1 I2C
    ├── uart/           # EUSART1
    ├── timer/          # TMR0, TMR2
    └── pwm/            # PWM3
```

## License

User application code (`main.c`, `bmi323.c`, `bmi323.h`, `bmm350.c`, `bmm350.h`, `X9C103S.c`, `X9C103S.h`) is released under the MIT License.

MCC-generated files in `mcc_generated_files/` are copyright Microchip Technology Inc. and may be used exclusively with Microchip products per their license terms.
