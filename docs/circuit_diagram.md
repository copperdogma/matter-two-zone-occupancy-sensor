# Matter Two-Zone Occupancy Sensor - Circuit Diagram

## ESP32-C3 SuperMini with Dual HC-SR501 PIR Sensors and Status LED

### Components
- ESP32-C3 SuperMini Board
- 2x HC-SR501 PIR Motion Sensors
- 5mm LED (Red, Green, or Blue)
- 220Ω Resistor
- Dupont Wires/Connectors
- Optional: Non-conductive hot glue for stability

### Wiring Diagram (ASCII)

```
                    FAR ZONE PIR              NEAR ZONE PIR
                   +---------------+          +---------------+
                   |  HC-SR501 #1  |          |  HC-SR501 #2  |
                   | +---+---+---+ |          | +---+---+---+ |
                   | |   |   |   | |          | |   |   |   | |
                   | +---+---+---+ |          | +---+---+---+ |
                   +---|---|---|---+          +---|---|---|---+
                      VCC OUT GND                VCC OUT GND
                       |   |   |                  |   |   |
                       |   |   |                  |   |   |
          +-------------------------+             |   |   |
          |            |   |   |    |             |   |   |
          |        5V -+---+   |    +-------------+   |   |
          |            |       |    |                 |   |
          |      GPIO3 -+------+    |                 |   |
          |            |            |                 |   |
          |      GPIO4 -+-----------+-----------------+   |
          |            |            |                     |
          |  ESP32-C3  |            |                     |
          |  SuperMini |            |                     |
          |            |            +---------------------+
          |            |            |
          |      GPIO5 -+--[220Ω]---+--- LED Anode (+)
          |            |            |
          |        GND -+------------+--- LED Cathode (-)
          |            |            |
          |            |            +--- PIR #1 GND
          |            |            |
          |            |            +--- PIR #2 GND
          +------------+
               |
               USB-C (Power)
```

### Connection Table

#### PIR Sensor #1 (Far Zone)
| PIR Pin | ESP32-C3 Pin | Wire Color (Suggested) |
|---------|--------------|------------------------|
| VCC     | 5V           | Red                    |
| OUT     | GPIO 3       | Yellow/Orange          |
| GND     | GND          | Black                  |

#### PIR Sensor #2 (Near Zone)
| PIR Pin | ESP32-C3 Pin | Wire Color (Suggested) |
|---------|--------------|------------------------|
| VCC     | 5V           | Red                    |
| OUT     | GPIO 4       | Green                  |
| GND     | GND          | Black                  |

#### LED Status Indicator
| LED Pin      | ESP32-C3 Pin | Notes              |
|--------------|--------------|-------------------|
| Anode (+)    | GPIO 5       | Via 220Ω resistor |
| Cathode (-)  | GND          | Direct connection |

### Assembly Notes

1. **Power Distribution**
   - Both PIR sensors share the 5V and GND pins from ESP32-C3
   - Consider using a breadboard or creating a simple power bus for cleaner wiring
   - ESP32-C3's 5V pin can supply enough current for both PIR sensors (each draws only 65μA)

2. **Signal Wires**
   - Keep PIR output wires (GPIO 3 and GPIO 4) away from power wires to reduce noise
   - Use quality Dupont connectors for reliable connections
   - Optional: Add small dab of hot glue at connector bases for mechanical stability

3. **LED Connection**
   - 220Ω resistor can be placed on either the anode or cathode side
   - Ensure correct polarity: longer LED leg is anode (+)
   - Test LED before final assembly

4. **PIR Sensor Configuration**
   - Set time delay potentiometer to minimum (firmware handles timeout)
   - Set sensitivity as needed for your application
   - Use jumper for H (repeatable trigger) mode

### GPIO Pin Selection Rationale

- **GPIO 3 & 4:** Safe pins for input on ESP32-C3, not used for boot/strapping
- **GPIO 5:** Safe for LED output, has PWM capability via LEDC
- **Avoided:** GPIO 2, 8, 9 (strapping pins), GPIO 18-19 (USB)

### Power Requirements

| Component          | Voltage | Current    | Power     |
|-------------------|---------|------------|-----------|
| ESP32-C3 (active) | 3.3V    | ~200mA     | 0.66W     |
| PIR Sensor #1     | 5V      | 65μA       | 0.0003W   |
| PIR Sensor #2     | 5V      | 65μA       | 0.0003W   |
| LED (typical)     | 2-3V    | 10-20mA    | 0.03W     |
| **Total**         | 5V      | **~220mA** | **~1.1W** |

USB-C provides 5V @ 500mA (2.5W), so power budget is comfortable with plenty of headroom.

### Power Architecture Design Decision

Both PIR sensors are powered directly from the ESP32-C3 SuperMini's 5V pin. This design choice is both safe and practical:

**Why This Works:**

1. **Negligible Load:** PIR sensors draw only 65μA each (0.13mA combined) - less current than typical pull-up resistors. This is essentially invisible to the power budget.

2. **Direct USB Connection:** The ESP32-C3's 5V pin connects directly to USB VBUS. You're not loading down a regulator; both the ESP32 and PIRs share the same USB 5V rail.

3. **Clean Power:** PIR sensors are passive infrared detectors - they don't generate switching noise, PWM, or draw pulsed current that could affect system stability.

4. **Proven Design:** Standard practice for powering multiple low-current sensors (<50mA each) from microcontroller boards.

**Design Advantages:**

- **Single USB-C Port:** One external connection for both power and programming
- **Easy Firmware Updates:** Flash new firmware without opening enclosure
- **Simpler Assembly:** Fewer wires, fewer failure points
- **No External PSU:** No need for USB hubs or separate power supplies
- **Cleaner Enclosure:** Single cable exit point

**When You'd Need Separate Power:**

Only consider external power if driving high-current devices:
- Motors or servos (>100mA)
- High-power LEDs or LED strips (>50mA)
- Multiple power-hungry sensors
- Devices with inrush current >100mA

At 0.13mA combined, the PIR sensors don't even approach these thresholds.

### Testing Procedure

1. **Initial Power Test**
   - Connect only ESP32-C3 via USB-C
   - Verify 5V and 3.3V outputs with multimeter
   - Check for stable voltage (no fluctuations)

2. **LED Test**
   - Connect LED with resistor
   - Flash simple blink firmware
   - Verify LED brightness and PWM functionality

3. **PIR Sensor Test**
   - Connect one PIR sensor at a time
   - Test motion detection with serial monitor
   - Verify clean HIGH/LOW transitions
   - Adjust sensitivity potentiometers as needed

4. **Full System Test**
   - Connect both PIR sensors
   - Flash two-zone firmware
   - Test each zone independently
   - Verify LED patterns (2 blinks vs 4 blinks)
   - Commission to Matter controller
   - Confirm both endpoints appear and respond correctly

### Troubleshooting

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| PIR not detecting | Wrong polarity | Verify VCC and GND connections |
| False triggers | Sensitivity too high | Adjust potentiometer |
| LED not lighting | Polarity reversed | Check anode/cathode |
| Unstable readings | Loose connections | Secure with hot glue |
| Both PIRs trigger together | Crossed wires | Verify GPIO 3 and 4 connections |
| Power issues | Too many devices | Verify total current <500mA (PIRs use only 0.13mA) |
