# LED Status Indicator

The Matter Two-Zone Occupancy Sensor includes an LED status indicator that provides visual feedback about device operation and which zone detected motion.

## Overview

The LED indicator uses PWM (Pulse Width Modulation) via the ESP32-C3's LEDC peripheral to control brightness and create zone-specific blink patterns.

## LED States

### Basic States
1. **Dim Static** (~10% brightness) - Device powered and idle, no motion detected
2. **Bright Static** (100% brightness) - Occupancy active in one or both zones
3. **Zone-Specific Blink Patterns** - Indicates which zone detected motion

### Zone-Specific Blink Patterns

**Far Zone Detection:**
- Blinks **2 times** rapidly (150ms on, 150ms off)
- Then stays bright for duration of occupancy

**Near Zone Detection:**
- Blinks **4 times** rapidly (150ms on, 150ms off)
- Then stays bright for duration of occupancy

## Behavior Sequence

### Far Zone Trigger
1. Start: LED is dim (idle state)
2. Motion in far zone detected
3. LED blinks 2 times rapidly
4. LED stays bright while occupied
5. After timeout (no motion): LED returns to dim

### Near Zone Trigger
1. Start: LED is dim (idle state)
2. Motion in near zone detected
3. LED blinks 4 times rapidly
4. LED stays bright while occupied
5. After timeout (no motion): LED returns to dim

### Both Zones Active
When both zones have active occupancy, the LED shows the pattern of whichever zone most recently detected motion.

## Hardware Setup

Default configuration:
- **GPIO Pin:** GPIO 5
- **Current-Limiting Resistor:** 220Ω in series

Wiring:
- LED anode (longer lead) connects through 220Ω resistor to GPIO 5
- LED cathode (shorter lead) connects to GND

## API Reference

### Public Functions

```c
// Initialize LED indicator
esp_err_t pir_led_indicator_init(int gpio_num);

// Set LED to dim state (idle/unoccupied)
esp_err_t pir_led_indicator_set_dim(void);

// Far zone pattern: 2 blinks then bright
esp_err_t pir_led_indicator_blink_far(void);

// Near zone pattern: 4 blinks then bright  
esp_err_t pir_led_indicator_blink_near(void);

// Set LED to full brightness (occupied state)
esp_err_t pir_led_indicator_set_bright(void);

// Deinitialize and free resources
esp_err_t pir_led_indicator_deinit(void);
```

## Configuration

Configure the LED GPIO pin via menuconfig:

```bash
idf.py menuconfig
```

Navigate to: **Occupancy Sensor Configuration → LED Indicator GPIO Pin Number**

Default settings:
- **GPIO pin:** 5
- **PWM frequency:** 5000 Hz
- **Dim brightness:** ~10% duty cycle
- **Bright brightness:** 100% duty cycle
- **Blink interval:** 150ms (per flash)

## Integration with Occupancy Detection

The LED is automatically controlled by the occupancy sensor callbacks:

```cpp
// In occupancy_sensor_notification callback:
if (occupancy) {
    if (zone == PIR_ZONE_FAR) {
        pir_led_indicator_blink_far();   // 2 blinks
    } else {
        pir_led_indicator_blink_near();  // 4 blinks
    }
} else {
    pir_led_indicator_set_dim();         // Return to idle
}
```

## Implementation Details

The LED driver uses FreeRTOS tasks and timers to create non-blocking blink patterns:

1. Blink commands spawn a temporary task
2. Task executes the appropriate number of blinks
3. Task automatically transitions to bright state
4. Task terminates after completing pattern

This approach ensures the blink patterns don't block the main occupancy detection logic.

## Troubleshooting

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| LED not lighting | Wrong polarity | Check anode/cathode orientation |
| LED too dim | Wrong resistor value | Verify 220Ω resistor |
| LED too bright | No resistor | Add 220Ω resistor |
| No blink pattern | Wrong GPIO | Verify GPIO 5 connection |
| Can't distinguish patterns | Blinks too fast | Adjust timing in code if needed |

## Future Enhancements

- Configurable blink counts via Kconfig
- RGB LED support for color-coded zones
- Brightness adjustment via Matter attribute
- Custom blink patterns for different events
