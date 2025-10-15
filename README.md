# Matter Two-Zone Occupancy Sensor

A Matter 1.4-compatible dual-zone occupancy sensor built with ESP32-C3 SuperMini and two HC-SR501 PIR sensors. Exposes two independent occupancy sensors via Matter protocol for advanced automation scenarios.

## Overview

This device uses two PIR motion sensors to create two distinct detection zones ("far" and "near"), each appearing as a separate Matter occupancy sensor. This enables sophisticated automation logic based on proximity and presence patterns.

**Example Use Case:** Interactive Halloween props like "Death, the Fortune-Telling Skeleton" - far zone triggers greeting, near zone starts full interaction.

## Quick Reference

```bash
# Build and flash
cd firmware
source ~/esp/esp-idf/export.sh
source ~/esp/esp-matter/export.sh
idf.py build
idf.py -p /dev/tty.usbmodemXXX flash monitor

# Current commissioning codes
QR Code: MT:Y.K90GSY00KA0648G00
Manual Code: 34970112332
```

## Features

- **Dual Zones:** Two independent occupancy sensors (far and near zones)
- **Detection:** HC-SR501 PIR sensors with adjustable range and sensitivity
- **Matter 1.4 Protocol:** Universal smart home integration (Apple Home, Google Home, Amazon Alexa)
- **Visual Feedback:** LED indicators with zone-specific patterns (2 blinks = far, 4 blinks = near)
- **Secure Commissioning:** QR code and manual pairing code setup
- **Power:** USB-C powered for continuous operation
- **Single-Cable Design:** Both PIRs powered from ESP32, no external PSU needed

## Hardware Components

| Component | Quantity | Details |
|-----------|----------|---------|
| **ESP32-C3 SuperMini** | 1 | Single-core 160 MHz, 400KB SRAM, 4MB Flash, Wi-Fi, BLE 5.0 |
| **HC-SR501 PIR** | 2 | Passive infrared motion sensors, adjustable sensitivity |
| **Status LED** | 1 | Visual feedback with zone-specific blink patterns |
| **220Ω Resistor** | 1 | For LED current limiting |

### Power Architecture

Both PIR sensors are powered directly from the ESP32-C3's 5V pin, which is safe and practical:

- **PIR power draw:** 2 × 65μA = 0.13mA (negligible)
- **Total system draw:** ~320mA (ESP32) + 0.13mA (PIRs) = ~320mA
- **USB 2.0 budget:** 500mA
- **Headroom:** 180mA (36% margin)

**Design Benefits:**
- Single USB-C connection for both power and programming
- No need for external power supply or USB hub
- Simple, reliable architecture
- Easy firmware updates without opening enclosure

## Hardware Connections

### ESP32-C3 SuperMini Connections

| ESP32-C3 Pin | Component Pin | Notes                   |
|--------------|---------------|-------------------------|
| GND          | PIR #1 GND    | Far zone sensor         |
| 5V           | PIR #1 VCC    | Far zone sensor         |
| GPIO 3       | PIR #1 Output | Far zone detection      |
| GND          | PIR #2 GND    | Near zone sensor        |
| 5V           | PIR #2 VCC    | Near zone sensor        |
| GPIO 4       | PIR #2 Output | Near zone detection     |
| GPIO 5       | LED Anode (+) | Through 220Ω resistor   |
| GND          | LED Cathode (-)| Ground connection       |

*For detailed wiring diagram, see [docs/circuit_diagram.md](docs/circuit_diagram.md)*

## Setup & Development

1. **Environment Setup**
   - Follow instructions in [SETUP.md](SETUP.md) to set up ESP-IDF, ESP-Matter SDK, and development certificates.

2. **Build & Flash**
   ```bash
   cd firmware
   source ~/esp/esp-idf/export.sh
   source ~/esp/esp-matter/export.sh
   # Build for ESP32-C3
   idf.py set-target esp32c3
   idf.py reconfigure
   idf.py build
   idf.py -p PORT flash monitor
   ```

3. **Commissioning**
   - After flashing, check serial monitor for QR code and pairing code
   - Use Apple Home or other Matter controller to add the device
   - You will see **two separate occupancy sensors** in your controller
   - Current codes: QR `MT:Y.K90GSY00KA0648G00`, Manual `34970112332`

## Project Structure

```
matter-two-zone-occupancy-sensor/
├── docs/                     # Documentation
│   ├── circuit_diagram.md   # Wiring diagrams
│   ├── led_indicator.md     # LED behavior details
│   └── datasheets/          # Component datasheets
├── firmware/                 # ESP32 firmware source code
│   ├── main/                 # Main application logic
│   │   ├── app_main.cpp     # Application entry point
│   │   └── drivers/         # Hardware drivers
│   └── CMakeLists.txt        # Build configuration
├── README.md                 # This file
└── SETUP.md                  # Environment setup guide
```

## Configuration

### Default Settings
- **Far Zone:** GPIO 3
- **Near Zone:** GPIO 4
- **Occupancy Timeout:** 10 seconds (configurable via Matter attribute `PIROccupiedToUnoccupiedDelay`)
- **LED Indicator:** GPIO 5 (external LED with 220Ω resistor)
  - 2 rapid blinks = far zone triggered
  - 4 rapid blinks = near zone triggered
  - Dim = idle, Bright = occupied

### Adjusting Configuration
Modify GPIO pins and timeouts via menuconfig:
```bash
idf.py menuconfig
# Navigate to: Occupancy Sensor Configuration
```

## Matter Integration

Each zone appears as a separate occupancy sensor endpoint in your Matter controller:
- **Endpoint 1:** Far Zone Occupancy Sensor
- **Endpoint 2:** Near Zone Occupancy Sensor

Both sensors share the same configurable timeout initially, but can be configured independently via Matter attributes.

## Commissioning Tips

1. **Always use current codes** from serial monitor after flashing
2. **Factory reset:** Hold BOOT button while power cycling (10 seconds)
3. **If commissioning fails:**
   - Remove device from controller app
   - Factory reset the device
   - Try again with current codes from serial monitor

## Technical Specifications

- **Matter Version:** 1.4 (Universal smart home compatibility)
- **Microcontroller:** ESP32-C3 SuperMini (160 MHz, 400KB SRAM, 4MB Flash)
- **Sensors:** 2x HC-SR501 PIR (adjustable range up to 7m, 120° coverage each)
- **Connectivity:** Wi-Fi 802.11 b/g/n, Bluetooth 5.0 LE
- **Power:** USB-C 5V, ~320mA total draw
- **Development:** ESP-IDF v5.4.1, ESP-Matter SDK

## Documentation

- [Setup Guide](SETUP.md) - Environment and hardware setup
- [Circuit Diagram](docs/circuit_diagram.md) - Detailed wiring
- [LED Indicator](docs/led_indicator.md) - LED behavior and patterns

## References

- **ESP32-C3 SuperMini:** [Pinout and specifications](https://github.com/sidharthmohannair/Tutorial-ESP32-C3-Super-Mini)
- **Matter Protocol:** [Matter 1.4 Specification](https://csa-iot.org/all-solutions/matter/)

## Author

Cam Marsollier
