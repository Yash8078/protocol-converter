# protocol-converter
esp32 based  transparent protocol converter 
# ESP32-S3 Based Ethernet-to-RS485 Protocol Converter

An embedded communication gateway built using the **ESP32-S3, W5500 Ethernet module, and ADM2483 isolated RS485 interface** to communicate with industrial devices using the Modbus RTU protocol.

The project is designed to provide Ethernet connectivity to RS485-based industrial meters, with a Selec MFM376 energy meter used as the initial test device.

## Project Overview

The protocol converter acts as a bridge between an Ethernet network and an RS485 serial communication bus. The ESP32-S3 handles communication, device configuration, and Modbus data acquisition.

## Hardware Components

- ESP32-S3-WROOM-1-N16R8
- W5500 Ethernet module
- ADM2483 isolated RS485 module
- Ethernet cable (RJ45)
- RS485 wiring and suitable power supplies

## System Architecture

```text
Laptop / Local Network
          |
      Ethernet
          |
   W5500 Ethernet
          |
      ESP32-S3
          |
     UART / TTL
          |
  ADM2483 Isolated
       RS485
          |
   Selec MFM376
     Energy Meter
```

## Hardware Connections

### ESP32-S3 to W5500

| W5500 Pin | ESP32-S3 GPIO |
|---|---|
| MOSI | GPIO 11 |
| MISO | GPIO 12 |
| SCK | GPIO 13 |
| CS | GPIO 10 |
| RST | GPIO 9 |
| VCC | 3.3V |
| GND | GND |

### ESP32-S3 to ADM2483

| ADM2483 Pin | ESP32-S3 Connection |
|---|---|
| DI | GPIO 16 (UART TX) |
| RO | GPIO 15 (UART RX) |
| Logic-side VCC | As required by the module |
| Logic-side GND | ESP32 logic GND |

### ADM2483 to Selec MFM376

| RS485 Module | Meter |
|---|---|
| A / D+ | RS485 A / + |
| B / D− | RS485 B / − |

Verify the actual breakout-board pinout, isolation requirements, and direction-control configuration before powering the circuit.

## Modbus RTU Configuration

| Parameter | Initial Configuration |
|---|---|
| Protocol | Modbus RTU |
| Meter | Selec MFM376 |
| Slave ID | 1 |
| Baud Rate | 9600 |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |

## Software Requirements

- Visual Studio Code
- PlatformIO IDE extension
- ESP32 platform for PlatformIO
- Required Ethernet and Modbus libraries


## Planned Development

- Configurable Modbus polling interval
- Support for multiple Modbus RTU devices
- Web-based device configuration
- Configurable network settings
- Industrial meter data acquisition
- Improved communication diagnostics and error handling

*Planned features will be marked as implemented once they have been developed and tested.*

## Project Status

**Development in progress.**

Hardware integration, firmware development, and Modbus communication testing are ongoing. Refer
