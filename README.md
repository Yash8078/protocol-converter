# ESP32-S3 Ethernet-to-RS485 Protocol Converter

**An embedded industrial communication gateway using ESP32-S3, W5500 Ethernet, and isolated RS485 communication.**

This project focuses on developing an Ethernet-to-RS485 protocol converter for communication with industrial devices using Modbus RTU. The development process includes hardware bring-up, SPI diagnostics, UART/RS485 communication testing, Ethernet integration, and web interface development.

The Selec MFM376 multifunction meter is the initial target device for Modbus communication and integration testing.

## Project Overview

The ESP32-S3 acts as the main controller, interfacing with a W5500 Ethernet module for network connectivity and an ADM2483 isolated RS485 interface for serial communication with industrial equipment.

The long-term objective is to develop a configurable gateway that can communicate with multiple RS485 devices and provide a web interface for device and network configuration.

## System Architecture

```text
        Laptop / Local Network
                  |
             Ethernet
                  |
          W5500 Ethernet
                  |
              SPI Bus
                  |
              ESP32-S3
                  |
            UART / TTL
                  |
       ADM2483 Isolated RS485
                  |
             RS485 Bus
                  |
         Selec MFM376 Meter
             Modbus RTU
```

## Hardware Components

| Component | Purpose |
|---|---|
| ESP32-S3-WROOM-1-N16R8 | Main controller |
| W5500 Ethernet module | Wired Ethernet connectivity |
| ADM2483 isolated RS485 module | Isolated serial communication |
| Selec MFM376 meter | Initial Modbus RTU test device |
| RJ45 Ethernet cable | Network connection |
| RS485 wiring | Communication between gateway and meter |

**Note:** Verify the exact breakout-board pinouts, supply requirements, and isolation arrangements before connecting the hardware.

## Hardware Pin Mapping

### ESP32-S3 to W5500

| Signal | ESP32-S3 GPIO |
|---|---|
| MOSI | GPIO 11 |
| MISO | GPIO 12 |
| SCK | GPIO 13 |
| CS | GPIO 10 |
| RESET | GPIO 9 |
| VCC | 3.3 V, subject to module specifications |
| GND | GND |

### ESP32-S3 to ADM2483

| Signal | ESP32-S3 Connection |
|---|---|
| UART TX | GPIO 16 |
| UART RX | GPIO 15 |
| Logic-side supply | According to module specifications |
| Logic-side ground | ESP32-S3 logic ground |

The UART TX and RX signals connect to the corresponding DI and RO signals on the RS485 interface. Direction-control pins must be configured according to the actual module design.

### ADM2483 to Selec MFM376

| RS485 Signal | Meter Connection |
|---|---|
| A / D+ | Corresponding RS485 A terminal |
| B / D− | Corresponding RS485 B terminal |

Confirm polarity against the device manuals; terminal naming can vary between manufacturers.

## Modbus RTU Configuration

| Parameter | Initial Setting |
|---|---|
| Target meter | Selec MFM376 |
| Protocol | Modbus RTU |
| Slave ID | 1 |
| Baud rate | 9600 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |

These are the initial target-meter settings. Update them if the meter or firmware configuration changes.

## Development and Testing Files

The repository currently contains individual Arduino sketches for different stages of development. Their exact functionality should be confirmed by reviewing each sketch.

| File | Development area |
|---|---|
| `w5500_spi_diagnostic.ino` | W5500 SPI diagnostics |
| `stage1testingw5500_01.ino` | Initial W5500 integration testing |
| `w5500_stage1_success.ino` | W5500 connectivity testing |
| `rxtest6483success.ino` | UART/RS485 receive testing |
| `esp32_ttl-rs4850_slave_success.ino` | TTL/RS485 communication testing |
| `slave_esp_connection_success.ino` | ESP32 connection testing |
| `protocol_converter_login.ino` | Login interface development |
| `web interface_development01.ino` | Web interface development |

These files represent separate development and testing sketches. Their presence does not necessarily mean all features have been integrated into a single firmware build.

## Software and Development Environment

- Visual Studio Code
- PlatformIO IDE or Arduino IDE, depending on the sketch
- ESP32 board support package
- Appropriate W5500 Ethernet library
- Modbus communication library, where required

Library names and versions should be documented alongside the firmware that actually uses them.

## Getting Started

1. Clone or download this repository.
2. Open the required `.ino` sketch in Arduino IDE, or configure the project for PlatformIO.
3. Install the libraries required by that particular sketch.
4. Select the correct ESP32-S3 board and configure the upload settings.
5. Verify the hardware wiring and power requirements.
6. Upload the selected test sketch.
7. Open the serial monitor and use the configured baud rate to inspect diagnostic output.
8. Test Ethernet connectivity or Modbus communication as appropriate for the selected sketch.

**Important:** Each sketch may have different pin definitions, libraries, and configuration requirements. Review its source code before uploading it to the hardware.

## Planned Features

The following are development objectives, not claims that every feature is already implemented:

- Ethernet-based communication using W5500
- Modbus RTU communication over RS485
- Configurable polling intervals
- Support for multiple Modbus devices
- Web-based device configuration
- Network configuration through a browser interface
- Device and communication diagnostics
- Extensible support for additional industrial meters

## Current Project Status

**Development and hardware integration in progress.**

The repository contains individual sketches for testing and developing the Ethernet, serial communication, and web interface components. The next stage is to consolidate the verified components into a maintainable firmware project and document the results of integration testing.

Update this section as each feature is verified on the physical hardware.

## Future Improvements

- Organize the firmware into reusable modules
- Consolidate the validated communication components
- Add structured error handling and recovery
- Document supported Modbus registers for each meter
- Add wiring diagrams and hardware photographs
- Add reproducible build and testing instructions
- Maintain versioned releases for stable firmware

## Contributing

Suggestions, bug reports, and improvements are welcome, subject to the project's ownership and licensing terms.

## License

No open-source license has been specified yet. Reuse and redistribution permissions must be established before a license is added.

## Author

**Yash Jain**

GitHub: [@Yash8078](https://github.com/Yash8078)

Repository: [protocol-converter](https://github.com/Yash8078/protocol-converter)

---

*This project is intended for embedded-systems development and industrial communication experimentation. Verify electrical connections, isolation requirements, and device configurations before deployment.*
