# FluidManifold Firmware

Firmware for the DwarfNova Fluid Manifold PCB Rev B (087-09002B).

## Hardware

| Item | Description |
|------|-------------|
| MCU | ESP32-S3-MINI-1-N4R2 (4 MB flash, 2 MB PSRAM) |
| PCB | DwarfNova Fluid Manifold Rev B (087-09002B) |
| Board ID | MANIFOLD (2) |

### Peripherals

| Device | Part | Interface | Address | Function |
|--------|------|-----------|---------|----------|
| U2 | SN65HVD485ED | UART2 (IO21/33/34) | — | RS-485 transceiver for Modbus RTU |
| U4 | TBD62003AFWGELCTND | GPIO | — | 7-ch NPN Darlington — NOZZLE1/SELECT, NOZZLE2/SELECT, WASTE_CATCHER, DROP_TRAY, SAMPLE_STATION LED |
| U5 | TBD62003AFWGELCTND | GPIO | — | 7-ch NPN Darlington — BOOST_REG, SAMPLE_REG, PINCH_FULL, PINCH_SOFT |
| U7 | ADS112C04 | I2C (IO1/2) | 0x40 | 24-bit ADC — sheath pressure (AIN0), Omega pressure (AIN1), laser temp thermistor (AIN2) |
| U9 | MCP4725 | I2C (IO1/2) | 0x60 | 12-bit DAC — analog pressure setpoint |
| U10 | SN65HVD485ED | UART1 (IO16/17/18) | — | RS-485 transceiver for Alicat regulator |
| U15 | EMC2302-1-AIZLTR | I2C (IO1/2) | 0x2E | Dual-channel PWM fan controller, ALERT on IO10 |
| U16 | MCP9808 | I2C (IO1/2) | 0x18 | Board temperature sensor |
| LED1 | WS2812B | RMT (IO6) | — | RGB serial status LED |

### GPIO Summary

| GPIO | Signal | Direction | Notes |
|------|--------|-----------|-------|
| IO1 | SCL | I/O | I2C clock |
| IO2 | SDA | I/O | I2C data |
| IO6 | LEDSI | O | WS2812B data (RMT) |
| IO7 | ERRLED | O, active low | Red error LED D2 |
| IO9 | VACUUM_SWITCH | I, active low | Fixed setpoint vacuum switch (J13), low = vacuum present |
| IO10 | FAN_ALERT | I, active low | EMC2302 ALERT output |
| IO11 | BOOST_REGULATOR | O | Boost pressure regulator solenoid (U5 1B, J10) |
| IO12 | SAMPLE_REGULATOR | O | Sample pressure regulator solenoid (U5 2B, J11) |
| IO13 | PINCH_FULL | O | Pinch valve full power (U5 3B, J12) |
| IO14 | PINCH_SOFT | O | Pinch valve soft mode (U5 6B+7B, J12) |
| IO16 | TXREG | O | Alicat RS-485 UART1 TX |
| IO17 | RXREG | I | Alicat RS-485 UART1 RX |
| IO18 | DIRREG | O | Alicat RS-485 direction, high = TX |
| IO21 | TX485 | O | Modbus RS-485 UART2 TX |
| IO33 | RX485 | I | Modbus RS-485 UART2 RX |
| IO34 | DIR485 | O | Modbus RS-485 direction, high = TX |
| IO35 | NOZZLE_1 | O | Nozzle 1 solenoid valve (U4 1B, J3) |
| IO36 | NOZZLE_SELECT_1 | O | Nozzle select 1 solenoid valve (U4 2B, J4) |
| IO37 | NOZZLE_2 | O | Nozzle 2 solenoid valve (U4 3B, J5) |
| IO38 | NOZZLE_SELECT_2 | O | Nozzle select 2 solenoid valve (U4 4B, J6) |
| IO39 | WASTE_CATCHER | O | Waste catcher solenoid valve (U4 5B, J7) |
| IO40 | DRIP_TRAY | O | Drip tray solenoid valve (U4 6B, J8) |
| IO41 | SAMPLE_STATION | O | Sample station illumination LED (U4 7B, J9) |

## Build

Built with PlatformIO (Arduino framework, espressif32 platform). Open the `FluidManifold` folder in VS Code with the PlatformIO extension installed.

```
pio run
```

**Pre-build step:** `extra_script.py` runs `src/genmodmap.py` before each build to auto-generate `src/modbusmap_gen.cpp` and `include/modbusmap_gen.h` from `config/modbusmap.xlsx`. Do not edit these generated files directly.

Key build flags (required):
```ini
-D USE_MODBUS_RTU=1
-D MODBUS_DISABLE_TCP=1
```

These prevent the modbus library from pulling in `WiFi.h`, which is not used on this board.

## Modbus Register Map

The board operates as a **Modbus RTU slave** on UART2 (RS-485). Slave address = MANIFOLD (2).

The register map is defined in `config/modbusmap.xlsx` and regenerated at build time. See `include/modbusmapuser.h` for the register address enums.

### Coils (FC01/05)

| Address | Name | Description |
|---------|------|-------------|
| 1 | ERROR | Error condition flag; write 0 to clear |
| 2 | RESET | Write 1 to reset the board |
| 3 | STATLED | Status LED (reserved) |
| 4 | ERRLED | Red error LED (D2) |
| 10 | BOOST_REG | Boost pressure regulator solenoid |
| 11 | SAMPLE_REG | Sample pressure regulator solenoid |
| 12 | PINCH_FULL | Pinch valve full power |
| 13 | PINCH_SOFT | Pinch valve soft mode |
| 14 | NOZZLE1 | Nozzle 1 solenoid |
| 15 | NOZZLE1_SELECT | Nozzle select 1 solenoid |
| 16 | NOZZLE2 | Nozzle 2 solenoid |
| 17 | NOZZLE2_SELECT | Nozzle select 2 solenoid |
| 18 | WASTE_CATCHER | Waste catcher solenoid |
| 19 | DROP_TRAY | Drip tray solenoid |
| 20 | SAMPLE_STATION | Sample station illumination LED |
| 22 | SHE_BOOST | Sheath boost (NC — not used on this board) |
| 23 | SHE_ATM | Sheath atmosphere (NC — not used on this board) |

### Discrete Inputs / Contacts (FC02)

| Address | Name | Description |
|---------|------|-------------|
| 1 | INERROR | Error status (read-only mirror of ERROR coil) |
| 2 | BOOTSW | Boot button state |
| 10 | VACUUM_SWITCH | Vacuum switch — 1 = vacuum present (active low hardware) |
| 11 | FANALERT | EMC2302 fan alert — 1 = alert active |

### Holding Registers (FC03/16)

| Address | Name | Description |
|---------|------|-------------|
| 1 | CONFIG | Board configuration flags (boardctl bits) |
| 2 | SERIALNUM_PROG | Program board serial number |
| 3 | REGBAUD | Alicat regulator serial baud rate |
| 4 | LEDCOLOR | WS2812B LED color |
| 6 | LEDBRIGHTPCT | WS2812B LED brightness (%) |
| 10 | FAN_RPM_SET1 | Fan 1 target RPM |
| 11 | FAN_RPM_SET2 | Fan 2 target RPM |
| 12 | CTLMODE | Control mode |
| 13 | TEMPTARGET | Temperature target (°C) |
| 15 | FANPID_INTERVAL | Fan PID interval (ms) |
| 16 | FANPID_KP | Fan PID proportional gain |
| 18 | FANPID_KI | Fan PID integral gain |
| 20 | FANPID_KD | Fan PID derivative gain |
| 22 | PRESSURE_SET_ANALOG | Analog pressure setpoint (PSI, via DAC) |
| 24 | PRESSURE_SET_DIGITAL | Alicat regulator setpoint (PSI, via RS-485) |

### Input Registers (FC04)

| Address | Name | Description |
|---------|------|-------------|
| 1 | BOARDID | Board ID (high byte) and revision (low byte) |
| 2 | FWVER | Firmware version — major (high byte), minor (low byte) |
| 3 | SERIALNUM | Board serial number |
| 4 | STATUSREG | Board status bits (boardstat) |
| 5 | ERRNUM | Current fault code |
| 10 | FAN_RPM_MON1 | Fan 1 current RPM (0 = off, 0xFFFF = fault) |
| 11 | FAN_RPM_MON2 | Fan 2 current RPM (0 = off, 0xFFFF = fault) |
| 12 | WIFI_RSSI | WiFi RSSI (dBm) |
| 13 | BOARD_TEMP | Board temperature (°C, float, 2 registers) |
| 15 | PRESSURE_MON_ANALOG | Sheath pressure — analog monitor (PSI, float, 2 registers) |
| 17 | PRESSURE_MON_DIGITAL | Sheath pressure — Alicat RS-485 (PSI, float, 2 registers) |
| 19 | OMEGA_PRESSURE | Omega pressure sensor (PSI, float, 2 registers, 0–150 PSI) |
| 21 | LASER_TEMP | Laser temperature via thermistor (°C, float, 2 registers) |

## Pressure Scaling

| Signal | Range | Notes |
|--------|-------|-------|
| Sheath pressure (analog) | 0–50 PSI | ADS112C04 AIN0, ADC correction factor 1.6804/1.12 |
| Omega pressure | 0–150 PSI | ADS112C04 AIN1 |
| Analog setpoint (DAC) | 0–50 PSI | MCP4725, DAC correction factor 1.109 |
| Digital setpoint (Alicat) | 0–50 PSI | Alicat RS-485, default 19200 baud |

## Revision History

| Version | Date | Notes |
|---------|------|-------|
| 0.10 | 2026-06-19 | Initial version, ported from NovaCart template, updated for Rev B PCB GPIO assignments |
