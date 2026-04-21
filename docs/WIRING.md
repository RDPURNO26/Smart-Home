<![CDATA[# Wiring Guide

Complete pin-by-pin wiring reference for the Smart Home IoT System.

---

## 📋 Components List

### Microcontrollers
- 1× NodeMCU ESP8266 (Master)
- 1× Arduino Uno/Nano (Slave)

### Sensors
- 1× DHT11 Temperature/Humidity sensor
- 1× MQ-8 Gas Sensor (hydrogen/combustible gas)
- 1× IR Flame Sensor (digital output)
- 1× Push Button (auto mode toggle / emergency reset)

### Actuators & Outputs
- 1× SG90 Servo Motor — door lock
- 1× 5V Relay Module — **ventilation fan only** (single relay)
- 1× Piezo Buzzer
- 1× 5mm LED — door status indicator
- 1× Main Light (LED or small bulb) — **direct to Arduino, no relay**
- 1× DC Fan (5V/12V)
- 1× NPN Transistor (BC547 or 2N2222) — fan PWM switching
- 1× 1N4007 Diode — flyback protection for fan
- 1× 220Ω Resistor — LED current limiting
- 1× 1kΩ Resistor — transistor base

### Power Supplies
- USB 5V for NodeMCU
- USB 5V or 7–12V barrel jack for Arduino
- External 12V for DC fan (if using 12V fan)

---

## 🚨 Safety Warnings

- **Disconnect power before wiring**
- **Double-check all connections before powering up**
- **Use current-limiting resistors for all LEDs**
- **Add a flyback diode across the fan motor terminals**
- **Ensure common ground between ALL boards and power supplies**

---

## NodeMCU ESP8266 (Master) — Pin Map

### Power & Ground
```
NodeMCU 3.3V → Sensor power (DHT11, Flame, MQ-8)
NodeMCU GND  → Common ground bus
```

### Sensors

| Sensor | NodeMCU Pin | Notes |
|---|---|---|
| DHT11 VCC | 3.3V | |
| DHT11 DATA | D5 (GPIO14) | |
| DHT11 GND | GND | |
| Flame VCC | 3.3V | |
| Flame DOUT | D6 (GPIO12) | Active LOW — goes LOW on flame detection |
| Flame GND | GND | |
| MQ-8 VCC | 3.3V | |
| MQ-8 AOUT | A0 | Analog reading |
| MQ-8 GND | GND | |
| Button Pin 1 | D3 (GPIO0) | Uses INPUT_PULLUP |
| Button Pin 2 | GND | |

### Actuators

| Component | NodeMCU Pin | Notes |
|---|---|---|
| Buzzer (+) | D0 (GPIO16) | |
| Buzzer (−) | GND | |
| Vent Relay IN | D7 (GPIO13) | Active LOW (`LOW` = relay ON) |
| Vent Relay VCC | 5V external | Or Arduino 5V if current permits |
| Vent Relay GND | GND | |

### I2C Bus (to Arduino)

```
NodeMCU D1 (GPIO5)  ──────  Arduino A5 (SCL)
NodeMCU D2 (GPIO4)  ──────  Arduino A4 (SDA)
NodeMCU GND         ──────  Arduino GND
```

> ⚠️ **Both boards MUST share a common ground.** Without it, I2C will be unreliable or fail entirely.

> 💡 Internal pull-ups are enabled by the Wire library. For I2C runs longer than ~30cm, add external 4.7kΩ pull-ups on SDA and SCL to 3.3V.

### NodeMCU Pin Summary

| Pin | GPIO | Component |
|---|---|---|
| D0 | 16 | Buzzer |
| D1 | 5 | I2C SCL |
| D2 | 4 | I2C SDA |
| D3 | 0 | Push Button |
| D5 | 14 | DHT11 Data |
| D6 | 12 | Flame Sensor DOUT |
| D7 | 13 | Ventilation Relay IN |
| A0 | — | MQ-8 Analog Out |
| 3.3V | — | Sensor power rail |
| GND | — | Common ground |

---

## Arduino Uno (Slave) — Pin Map

### Power & Ground
```
Arduino 5V  → Servo VCC, LED (via resistor)
Arduino GND → Common ground bus
External 12V+ → DC fan positive (if 12V fan)
```

### Actuators

| Component | Arduino Pin | Type | Notes |
|---|---|---|---|
| **Main Light (+)** | **D9** | Digital Out | **DIRECT** — no relay. Use 220Ω series resistor if driving an LED |
| Main Light (−) | GND | | |
| Servo Signal (orange) | D10 | Digital Out | 0° = locked, 180° = unlocked |
| Servo VCC (red) | 5V | Power | **Never power from NodeMCU 3.3V** |
| Servo GND (brown) | GND | | |
| Door Status LED (+) | D6 | Digital Out | Through 220Ω resistor |
| Door Status LED (−) | GND | | |
| NPN Transistor Base | D5 | PWM Out | Through 1kΩ resistor — fan speed control |
| NPN Transistor Emitter | GND | | |
| NPN Transistor Collector | Fan (−) wire | | |

### Arduino Pin Summary

| Pin | Component | Type |
|---|---|---|
| D5 | NPN Base (via 1kΩ) — Fan PWM | PWM Output |
| D6 | Door Status LED (via 220Ω) | Digital Output |
| D9 | Main Light (direct, via 220Ω if LED) | Digital Output |
| D10 | Servo Signal | Digital Output |
| A4 | I2C SDA | Communication |
| A5 | I2C SCL | Communication |
| 5V | Servo power, LED power | Power |
| GND | Common ground | Ground |

---

## Detailed Circuit Guides

### Main Light (Direct to Arduino D9)
```
Arduino D9 ── 220Ω ── LED (+) ── LED (−) ── GND
```
- For a small LED (≤20mA), this works directly
- For higher-power LED strips, add a transistor driver circuit (same as fan, on a separate pin)

### Fan Transistor Circuit (NPN — BC547 or 2N2222)
```
External 12V+ ──┬── Fan RED (+)
                 │
                 └── Diode Cathode ─┐ (1N4007, stripe toward 12V+)
                                    │
Fan BLACK (−) ──┬── NPN Collector   │
                └── Diode Anode ────┘
                
NPN Emitter ──── GND (common ground)
NPN Base ─── 1kΩ ─── Arduino D5 (PWM)
```

> ⚠️ **The 1N4007 diode is critical.** When the fan motor turns off, it generates a voltage spike (back-EMF) that can destroy the transistor. The diode absorbs this spike. Cathode (stripe) goes toward the positive supply.

> 💡 The Timer0 prescaler on the Arduino is modified in firmware to generate ~31.25kHz PWM on pin 5, which eliminates audible fan whine.

### Servo Motor (Door Lock)
```
Servo Red (VCC)    → Arduino 5V
Servo Brown (GND)  → Arduino GND
Servo Orange (Sig) → Arduino Pin 10
```
- 0° = Locked position
- 180° = Unlocked position

### DHT11 Temperature Sensor
```
DHT11 Pin 1 (VCC)  → NodeMCU 3.3V
DHT11 Pin 2 (Data) → NodeMCU D5 (GPIO14)
DHT11 Pin 3 (NC)   → Not connected
DHT11 Pin 4 (GND)  → GND
```

### MQ-8 Gas Sensor
```
MQ-8 VCC  → 3.3V (or 5V depending on breakout board)
MQ-8 GND  → GND
MQ-8 AOUT → NodeMCU A0
MQ-8 DOUT → Not used (analog threshold in firmware)
```

> ⚠️ **Preheat required.** The MQ-8 needs 60 seconds to stabilize. The firmware handles this with `delay(60000)` in `setup()`.

> ⚠️ NodeMCU A0 accepts 0–3.3V max. Most MQ-8 breakout boards include an onboard voltage divider. Verify your module's AOUT doesn't exceed 3.3V.

### Flame Sensor (IR Digital)
```
Flame VCC  → NodeMCU 3.3V
Flame GND  → GND
Flame DOUT → NodeMCU D6 (GPIO12)
```
- **Active LOW** — output goes LOW when flame is detected
- Adjust the onboard potentiometer to set detection sensitivity/range

### Ventilation Fan Relay
```
Relay Module:
  VCC → External 5V (or Arduino 5V if current permits)
  GND → Common GND
  IN  → NodeMCU D7 (GPIO13)
  COM → Power source positive (DC+ or AC Live)
  NO  → Ventilation fan positive wire
  Fan negative → Power source negative (GND or Neutral)
```

> 💡 **Only one relay in the entire system** — used exclusively for the ventilation/exhaust fan. The main room light is driven directly from Arduino D9.

---

## ⚡ Power Distribution

```
Power Architecture:
├── USB 5V → NodeMCU
│     └── 3.3V regulator → DHT11, Flame Sensor, MQ-8
│
├── USB / Barrel Jack → Arduino Uno
│     └── 5V rail → Servo, Door LED, Main Light LED
│
└── External 12V Supply
      └── DC Fan positive (switched by NPN transistor on Arduino D5)

⚠️ GROUNDING RULE:
All GND pins MUST be connected together:
  NodeMCU GND ── Arduino GND ── External 12V GND ── Relay GND
```

---

## 🛠️ Assembly Steps

### Phase 1: Microcontrollers
1. Place NodeMCU and Arduino on breadboard or PCB
2. Connect all GND pins together (common ground bus)
3. Wire I2C: NodeMCU D1 → Arduino A5 (SCL), NodeMCU D2 → Arduino A4 (SDA)

### Phase 2: Sensors (NodeMCU side)
1. DHT11: VCC → 3.3V, DATA → D5, GND → GND
2. Flame: VCC → 3.3V, DOUT → D6, GND → GND
3. MQ-8: VCC → 3.3V, AOUT → A0, GND → GND
4. Button: one pin to D3, other to GND

### Phase 3: Fan Transistor Circuit (Arduino side)
1. Solder/connect 1kΩ resistor to NPN transistor base lead
2. Connect resistor other end to Arduino D5
3. Connect NPN collector to fan's black (−) wire
4. Connect NPN emitter to common GND
5. Connect fan's red (+) wire to external 12V+
6. Add 1N4007 diode across fan terminals (cathode/stripe toward 12V+, anode toward collector)

### Phase 4: Remaining Actuators (Arduino side)
1. Servo: red → 5V, brown → GND, orange → D10
2. Door LED: D6 → 220Ω → LED (+) → LED (−) → GND
3. Main light: D9 → 220Ω → light (+) → light (−) → GND
4. Buzzer: NodeMCU D0 → buzzer (+), buzzer (−) → GND
5. Vent relay: NodeMCU D7 → relay IN, wire fan through relay COM/NO

### Phase 5: Power Up & Test
1. Connect USB to NodeMCU
2. Connect USB/barrel to Arduino
3. Connect external 12V to fan circuit
4. Open Serial Monitor at 115200 baud
5. Test each component individually with serial commands (see [COMMANDS.md](COMMANDS.md))

---

## ✅ Verification Checklist

- [ ] All GNDs connected together (common ground bus)
- [ ] I2C connections secure (SCL to SCL, SDA to SDA)
- [ ] NPN transistor orientation correct (Collector, Base, Emitter — check datasheet)
- [ ] 1N4007 diode polarity correct (stripe toward fan +12V)
- [ ] 220Ω resistor on door LED (D6)
- [ ] 220Ω resistor on main light (D9) if using bare LED
- [ ] 1kΩ resistor on transistor base (D5)
- [ ] No shorts between power rails
- [ ] Main light on D9 — **no relay**, direct connection
- [ ] Servo powered from Arduino 5V, **not** NodeMCU 3.3V
- [ ] Only one relay in system — for ventilation fan on NodeMCU D7

---

## 🐛 Troubleshooting

### Main light not working
- Check D9 with multimeter — should toggle between 0V and 5V
- Verify 220Ω series resistor is in place
- If using a high-power light, you need a transistor driver — direct connection only works for small LEDs (≤20mA)

### Fan not spinning
- Measure voltage across fan terminals (should match external supply when D5 is HIGH)
- Check base voltage at transistor from D5 (should see PWM signal)
- Verify transistor isn't burnt — check with multimeter in diode mode
- Ensure flyback diode is correctly oriented

### I2C communication failure
- Run an I2C scanner sketch on NodeMCU to detect slave
- Confirm slave address is 8 in both firmwares
- Check that SDA/SCL aren't swapped
- Add external 4.7kΩ pull-ups if wires are long

### Buzzer not sounding
- Verify D0 (GPIO16) — note this pin has unique behavior on ESP8266
- Test with `BUZZER_TEST` serial command
- Check buzzer polarity

### Servo jittering
- Servo draws too much current from the 5V rail — add a separate 5V supply
- Keep servo wires short to reduce noise
- Add a 100µF capacitor across servo VCC/GND
]]>
