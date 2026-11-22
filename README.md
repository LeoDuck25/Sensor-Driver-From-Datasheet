# MPU-6050 Driver From the Datasheet (ESP32, ESP-IDF, C)

[![host-tests](https://github.com/LeoDuck25/Sensor-Driver-From-Datasheet/actions/workflows/tests.yml/badge.svg)](https://github.com/LeoDuck25/Sensor-Driver-From-Datasheet/actions/workflows/tests.yml)

A bare-metal-style I2C driver for the InvenSense MPU-6050 IMU, written against the register map with **no vendor or Arduino libraries**. Raw-to-physical conversion lives in a hardware-independent C module that is unit-tested on a PC and in CI.

---

## Features

- **Device bring-up:** `WHO_AM_I` identity check (expects `0x68`), wake from sleep, PLL clock source, accelerometer range configuration
- **Burst read:** a single I2C transaction reads accel X/Y/Z + temperature (`0x3B`–`0x42`, 8 bytes) so all values come from the same sampling instant
- **Correct signed decoding:** big-endian byte pairs → `int16_t` (two's complement), done portably
- **Unit conversion:** accelerometer → g for all four full-scale ranges (±2/4/8/16 g); temperature → °C per datasheet formula
- **Host-side unit tests:** conversion math tested on PC with known vectors and edge cases (`0x8000`, `0x7FFF`, sign handling)
- **CI:** GitHub Actions builds and runs the tests on every push

## Architecture

```
main/
├── main.c          # ESP-IDF: I2C bus setup, device init, read loop
├── sensor_math.c   # Pure C: byte combining + unit conversion (no hardware deps)
└── sensor_math.h
test/
└── test_sensor_math.c   # Host-side unit tests (gcc)
Makefile                 # make test
.github/workflows/       # CI
```

Hardware stays in `main.c`; Math stays in `sensor_math.c` so it can be verified without a board.

## Hardware

| Signal | ESP32 pin | MPU-6050 |
|--------|-----------|----------|
| SDA    | GPIO 21   | SDA      |
| SCL    | GPIO 22   | SCL      |
| 3V3    | 3V3       | VCC      |
| GND    | GND       | GND      |

- I2C @ 100 kHz, 7-bit address `0x68` (AD0 low)
- Pull-ups: breakout-board resistors (ESP32 internal pull-ups also enabled)

## Build & Run

**Firmware** (ESP-IDF v5.x):
```bash
idf.py set-target esp32
idf.py build flash monitor
```

**Unit tests** (any machine with gcc):
```bash
make test
```

## How It Works

**Reading a register** is a two-phase I2C transaction:

```
START | 0x68+W | ACK | reg addr | ACK | REPEATED START | 0x68+R | ACK | data... | NACK | STOP
        master   slave  master    slave                  master   slave  slave    master
```

- The **repeated start** keeps bus ownership between setting the register pointer and reading, so the read is atomic.
- The master **NACKs the last byte** to tell the slave to release SDA.
- The register pointer **auto-increments**, so one transaction reads all 8 bytes.

**Decoding:** each axis is two registers, high byte first, two's complement.
```c
int16_t raw = (int16_t)(((uint16_t)hi << 8) | lo);
float   g   = raw / 16384.0f;   // ±2 g range: 16384 LSB/g
```

## Verification

| Check | Method | Status |
|-------|--------|--------|
| Conversion math | Host unit tests (known vectors, edge cases) | ✅ Passing in CI |
| Bus transactions | Logic analyzer capture, decoded and annotated against the code | ⏳ Pending hardware |
| Physical accuracy | 6-position gravity test, \|a\| ≈ 1.00 g in each pose | ⏳ Pending hardware |
| Reference agreement | Mean of 100 samples per pose vs. reference library, same config | ⏳ Pending hardware |

### Results

_To be filled with measured data._

| Pose | Driver (g) X / Y / Z | Reference (g) X / Y / Z | Δ (mg) | \|a\| (g) |
|------|----------------------|-------------------------|--------|-----------|
| +Z up | | | | |
| −Z up | | | | |
| +X up | | | | |
| −X up | | | | |
| +Y up | | | | |
| −Y up | | | | |

## Roadmap

- [x] I2C bus + device init, identity check
- [x] Burst read and signed decoding
- [x] Unit conversion + host unit tests + CI
- [ ] Annotated logic analyzer capture
- [ ] Reference-library comparison and 6-position test
- [ ] Gyroscope conversion
- [ ] Structured error handling (retry, fault state instead of abort)
- [ ] UART or SPI driver for a second peripheral

## References

- MPU-6000/MPU-6050 Product Specification
- MPU-6000/MPU-6050 Register Map and Descriptions
- ESP-IDF I2C Master Driver documentation