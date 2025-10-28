# Halloween Decoration Build - "Bug Jar" Variant

## Concept

Transform the two-zone occupancy sensor into a Halloween decoration that hides the electronics in plain sight. The device appears as a creepy specimen jar containing preserved "bug parts" but secretly monitors motion zones while providing ambient colored lighting effects.

## Design Overview

**Container**: Clear plastic jar with two circular holes cut in the front for PIR sensor "eyes"

**Aesthetic**: 
- Paint a creepy bug face/label around the PIR sensor holes
- Fill interior with polyester fill (pillow stuffing) to diffuse LED light
- Internal electronics completely hidden by stuffing
- Glowing colored light creates "mysterious specimen" effect
- USB-C cable exits from bottom/back of jar

**Functional Benefits**:
- Color-coded status visible at a glance
- Appears to be pure decoration to observers
- Fully operational Matter occupancy sensor
- Can trigger Halloween automation (sound effects, other props)

## LED Implementation Options

### Option 1: Three Separate Colored LEDs

**Hardware Requirements:**
- 3× 5mm LEDs (suggested colors: Yellow, Green, Blue)
- 3× 220Ω resistors
- 3× GPIO pins (GPIO 5, 6, 7)

**Wiring:**
```
ESP32-C3 GPIO5 → [220Ω] → Yellow LED Anode
ESP32-C3 GPIO6 → [220Ω] → Green LED Anode  
ESP32-C3 GPIO7 → [220Ω] → Blue LED Anode

All LED Cathodes → ESP32-C3 GND
```

**State Mapping:**
| State | LED(s) Active | Color | Effect |
|-------|--------------|-------|---------|
| Idle (no motion) | Yellow | Sickly amber | Dim pulse (breathing) |
| Near Zone Occupied | Green | Toxic green | Bright pulse (fast) |
| Far Zone Occupied | Blue | Eerie blue | Steady bright |
| Both Zones | Green + Blue | Cyan mix | Alternating pulse |

**Advantages:**
- Simple circuit, no special libraries needed
- Three separate light sources create interesting spatial effects when diffused
- Can mix colors by activating multiple LEDs
- Easy PWM control using existing LEDC peripheral
- Each LED independently controllable for complex patterns

**Disadvantages:**
- Uses three GPIO pins
- Limited to additive color mixing
- More complex wiring in confined jar space
- Limited color palette (3 colors + 4 combinations)

**Assembly Notes:**
- Mount LEDs at different heights/positions inside jar for better diffusion
- Ensure polyester fill doesn't press directly on LED surfaces (fire safety)
- Test color combinations before sealing jar

---

### Option 2: Single WS2812B RGB LED (Recommended)

**Hardware Requirements:**
- 1× WS2812B addressable RGB LED (or NeoPixel)
- 1× GPIO pin (GPIO 5)
- Optional: 300-500Ω resistor for data line signal integrity
- Optional: 100μF capacitor across power rails for stability

**Wiring:**
```
ESP32-C3 5V    → WS2812B VCC
ESP32-C3 GND   → WS2812B GND
ESP32-C3 GPIO5 → [330Ω optional] → WS2812B DIN
```

**State Mapping:**
| State | RGB Value | Color | Effect |
|-------|-----------|-------|---------|
| Idle (no motion) | (60, 30, 0) | Sickly amber | Slow breathing pulse |
| Near Zone Occupied | (50, 255, 30) | Radioactive green | Fast pulse |
| Far Zone Occupied | (0, 100, 255) | Electric blue | Steady glow |
| Both Zones | (255, 0, 255) | Alarming magenta | Rapid alternating |

**Alternate Color Schemes:**

*Toxic Waste Theme:*
- Idle: (70, 40, 0) - Dim yellow-orange
- Near: (100, 255, 50) - Bright toxic green
- Far: (0, 150, 200) - Cold cyan
- Both: (200, 255, 0) - Intense yellow-green

*Haunted Theme:*
- Idle: (20, 0, 40) - Deep purple
- Near: (0, 255, 100) - Ghostly green
- Far: (255, 50, 0) - Blood red
- Both: Slow rainbow cycle

**Advantages:**
- **Single GPIO pin** - minimal wiring
- **Infinite color options** - any RGB combination
- **Smooth transitions** - can fade between states
- **Central point source** - optimal for even diffusion through polyester
- **Built-in effects** - breathing, pulsing, color shifts
- **Future expandable** - can chain multiple WS2812B LEDs
- **Perfect for jar aesthetic** - single glowing orb creates "mysterious specimen" vibe

**Disadvantages:**
- Requires addressable LED library (esp_led_strip or FastLED)
- Slightly more complex firmware
- 5V logic on 3.3V GPIO (usually works fine, but technically out of spec)

**Software Integration:**

The ESP-IDF already has WS2812B support via the `led_strip` component. Integration points:

```cpp
// In app_main.cpp callback:
void occupancy_sensor_notification(uint16_t endpoint_id, bool occupancy, void *user_data) {
    const char *zone = static_cast<const char *>(user_data);
    
    if (occupancy) {
        if (is_far_zone(zone)) {
            rgb_led_set_color(0, 100, 255);      // Electric blue
            rgb_led_set_effect(EFFECT_STEADY);
        } else {
            rgb_led_set_color(50, 255, 30);      // Radioactive green
            rgb_led_set_effect(EFFECT_FAST_PULSE);
        }
    } else {
        // Check if other zone still occupied
        if (no_zones_occupied()) {
            rgb_led_set_color(60, 30, 0);        // Sickly amber
            rgb_led_set_effect(EFFECT_BREATHE);
        }
    }
}
```

**Assembly Notes:**
- Position WS2812B in center of jar for even light distribution
- Ensure polyester fill is evenly distributed around LED
- Leave small air gap around LED for heat dissipation (though minimal heat at these levels)
- Test colors through polyester fill before finalizing - colors will appear softer/pastel

---

## Physical Construction

### Materials Needed
- Clear plastic jar (wide mouth, 8-16 oz size)
- Acrylic paint (black, green, or appropriate "bug jar" colors)
- Polyester fill / pillow stuffing (white or natural)
- Drill or rotary tool with 20mm hole saw (for PIR sensor openings)
- Hot glue gun
- Small zip ties or cable management clips
- Optional: Rubber grommet for USB-C cable exit

### Assembly Steps

1. **Cut PIR Sensor Holes**
   - Mark two circles on jar front, spaced for "eyes" appearance
   - PIR sensor dome is ~20mm diameter - cut holes slightly smaller (~18mm)
   - Use fine sandpaper to smooth edges
   - Test-fit PIR sensors - should press-fit snugly

2. **Paint Jar Label**
   - Sketch bug face or specimen label around PIR holes
   - Use acrylic paint or permanent markers
   - Let dry completely before proceeding
   - Optional: Add text like "SPECIMEN #037" or "ENCHANTED BUG PARTS"

3. **Prepare Base/Lid**
   - Drill small hole in jar bottom or lid for USB-C cable
   - Install rubber grommet (optional but recommended)
   - Mount ESP32-C3 to jar lid with hot glue or double-sided tape

4. **Wire Everything**
   - Complete all electrical connections per chosen LED option
   - Test functionality before sealing
   - Use cable ties to organize wires compactly

5. **Install PIR Sensors**
   - Position PIR sensors to protrude through "eye" holes
   - Secure with hot glue around edges
   - Ensure sensors have 120° field of view outward
   - Verify sensors don't touch each other or create shorts

6. **Stuff with Polyester Fill**
   - Fill jar loosely around electronics
   - Test LED visibility - should diffuse nicely, not be blocked
   - Don't pack too tightly - allow some air circulation
   - LED should be visible but not directly exposed

7. **Final Assembly**
   - Close jar and secure lid
   - Route USB-C cable through bottom hole
   - Test all functions
   - Commission to Matter controller

### PIR Sensor "Eye" Positioning

**Recommended Layout:**
```
           ___________
          /           \
         |             |
         |   ●     ●   |  ← PIR sensors (18-20mm apart)
         |  FAR   NEAR |     Left = Far Zone
         |             |     Right = Near Zone
         |   \  ~  /   |  ← Paint creepy face around them
          \___________/
```

Position sensors horizontally at same height, pointing forward. This gives:
- Far Zone (left): Monitors entryway, approaching visitors
- Near Zone (right): Monitors immediate area in front of jar

## Automation Ideas

Once commissioned as a Matter device, the sensor can trigger:

1. **Sound Effects**: Different sounds for near vs far detection
2. **Other Props**: Activate animatronics when someone approaches
3. **Lighting Scenes**: Trigger room color changes based on zone
4. **Recording**: Log visitor patterns on Halloween night
5. **Cascading Effects**: Chain multiple bug jars for sequential activation

## Power Considerations

**USB Power Options:**
- USB wall adapter (recommended - 5V 1A minimum)
- USB battery bank (for untethered placement)
- USB extension cable for remote power source

**Runtime on Battery:**
With 10,000mAh USB battery bank:
- Idle (dim LED): ~40-45 hours
- Active (bright LED + WiFi): ~30-35 hours
- More than enough for Halloween night

## Safety Notes

- **Fire Safety**: Polyester fill is flammable - keep away from high-power LEDs. At 20mA (single LED), fire risk is negligible, but still ensure LEDs don't directly contact stuffing
- **Ventilation**: ESP32-C3 generates minimal heat, but ensure some air circulation
- **Water**: Not weatherproof - keep indoors or in covered area
- **Tripping Hazard**: Secure USB cable to prevent trips in dark

## Bill of Materials

### For WS2812B Version (Recommended)
| Item | Quantity | Estimated Cost |
|------|----------|----------------|
| ESP32-C3 SuperMini | 1 | $4-6 |
| HC-SR501 PIR Sensors | 2 | $2-4 |
| WS2812B RGB LED | 1 | $0.50-1 |
| Clear Plastic Jar | 1 | $3-5 |
| Polyester Fill | Small bag | $5-8 |
| Dupont Wires | Assorted | $3-5 |
| Hot Glue Sticks | Few | $2-3 |
| Acrylic Paint | As needed | $3-8 |
| USB-C Cable | 1 | $3-5 |
| **Total** | | **$25-45** |

### For Three-LED Version
Same as above, but replace WS2812B with:
| Item | Quantity | Estimated Cost |
|------|----------|----------------|
| 5mm LEDs (Yellow/Green/Blue) | 3 | $0.30-1 |
| 220Ω Resistors | 3 | $0.15-0.50 |

## Firmware Modifications Required

### For WS2812B Support

1. **Add led_strip component** to `main/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "app_main.cpp" 
         "drivers/pir.cpp"
         "drivers/rgb_led.cpp"  # New file
    INCLUDE_DIRS "."
    PRIV_REQUIRES led_strip
)
```

2. **Create new driver** `drivers/rgb_led.cpp` and `drivers/rgb_led.h`

3. **Integrate with PIR callbacks** in `app_main.cpp`

4. **Add Kconfig options** for color customization

### For Three-LED Support

1. **Extend existing LED driver** to support three LEDC channels

2. **Add color mixing logic** for combined states

3. **Update LED indicator API** to support multi-color patterns

---

## Testing Checklist

- [ ] PIR sensors detect motion correctly in both zones
- [ ] LED colors match expected states
- [ ] Color diffusion through polyester looks good
- [ ] No overheating after 30 minutes
- [ ] Matter commissioning successful
- [ ] Both endpoints appear in Home app/controller
- [ ] Automation triggers work correctly
- [ ] Painted design looks appropriately creepy
- [ ] USB cable secured and not a trip hazard
- [ ] Jar is stable and won't tip over

## Future Enhancements

- **Multiple Jars**: Create a collection with different colors/effects
- **Sound Integration**: Add I2S amplifier and speaker for audio feedback
- **Animation Modes**: Halloween party mode with special patterns
- **Battery Indicator**: Show low battery warning via color shift
- **Configurable Colors**: Adjust RGB values via Matter attributes or web interface

---

## Conclusion

This build transforms a functional occupancy sensor into an entertaining Halloween decoration while maintaining full smart home integration. The single WS2812B LED option is recommended for ease of assembly and maximum visual effect, though the three-LED version offers a simpler firmware path.

Time investment: 2-3 hours for complete assembly and testing.

Perfect for last-minute Halloween projects or as a permanent decorative sensor for year-round use with seasonal color schemes!

