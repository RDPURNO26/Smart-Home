<![CDATA[# About This Project

## The Story Behind Smart Home IoT

### 💡 Conception

The idea started simply: _"What if my room could protect itself?"_

Not just turn lights on and off — but actually detect danger, respond instantly, and work even when the internet is down. Every commercial smart home system I looked at had the same fatal flaw: pull the WiFi plug and the "smart" home becomes a dumb one. I wanted something that would never fail in an emergency.

That question led to weeks of research into IoT architectures, sensor physics, emergency response protocols, and microcontroller communication — and eventually to this project.

### 🔬 Research Phase

Before writing a single line of code, I spent significant time understanding:

- **How gas sensors actually work** — The MQ-8 needs a 60-second preheat cycle before its readings stabilize. I learned this the hard way when early prototypes would trigger false alarms immediately on power-up. The moving average filter was added after testing showed single-read spikes from electrical noise.

- **Why I2C over SPI or UART** — UART would have been simpler, but I needed the ESP8266's single hardware UART for serial debugging. SPI requires more pins than the NodeMCU has free after all sensors are connected. I2C uses just two wires and supports the simple command-byte protocol perfectly.

- **PWM fan control quirks** — Standard 490Hz Arduino PWM makes fans whine audibly. By modifying the Timer0 prescaler to achieve ~31.25kHz, the fan runs silently at all speed levels. The kick-start pulse (500ms at full power before dropping to target speed) was needed because the motor couldn't overcome static friction at lower PWM values.

- **Active-LOW relay modules** — Most cheap relay modules from AliExpress/Amazon use active-LOW logic. This is counterintuitive (`LOW` = relay ON, `HIGH` = relay OFF) and caused several debugging sessions before I standardized the `RELAY_ON`/`RELAY_OFF` defines.

- **Sinric Pro vs Blynk vs Arduino IoT Cloud** — I evaluated all three. Blynk's free tier was too limited. Arduino IoT Cloud required their specific board configurations. Sinric Pro offered free-tier support with native Alexa/Google Home bridging out of the box, making it the clear winner.

### 🔧 Development Challenges

**Challenge 1: Emergency systems must work offline**
The biggest architectural decision was making gas and flame detection completely independent of WiFi. The main loop runs a priority-based scheduler where emergency checks happen every 100ms regardless of connection state. The `checkConnectionStatus()` function gracefully degrades — if WiFi drops, all cloud-dependent code paths are skipped, but the safety pipeline never stops.

**Challenge 2: Door auto-lock timer with warning system**
Users needed visual and temporal feedback before auto-locking. The solution was a 60-second timer with warning LED blinks starting at 50 seconds. The Arduino slave handles the blink patterns independently (non-blocking via `millis()`), so the master can continue its sensor polling without delays.

**Challenge 3: Fan speed and servo commands during emergencies**
During a gas or fire emergency, the system must prevent any user-initiated actions that could be dangerous (turning on electrical devices near a gas leak). Every control function now checks `gasEmergency` and `fireEmergency` flags before executing, and the emergency handler saves pre-emergency states so the system can restore gracefully after the all-clear.

**Challenge 4: I2C reliability**
Early prototypes occasionally dropped I2C commands. The solution was keeping all Wire transmissions short (1-2 bytes) and avoiding blocking operations in the `receiveEvent` interrupt handler on the Arduino slave side. Long operations like `delay()` are never called inside ISRs.

### 🛠️ Technologies Chosen and Why

| Decision | Choice | Reasoning |
|---|---|---|
| Master MCU | NodeMCU ESP8266 | Built-in WiFi, 4MB flash, huge community, dirt cheap (~$3) |
| Slave MCU | Arduino Uno | 5V logic for relay/servo compatibility, dedicated PWM timers |
| Cloud | Sinric Pro | Free tier, Alexa + Google Home native, WebSocket (low latency) |
| Gas Sensor | MQ-8 | Hydrogen detection for general combustible gas, analog output for threshold tuning |
| Flame Sensor | IR Digital | Instant binary detection, no complex calibration needed |
| Door Lock | Servo Motor | Simple 0°/180° mapping to locked/unlocked, no motor driver needed |
| Buzzer | Active Piezo | `tone()` support for distinct alarm patterns (siren vs steady) |
| Communication | I2C | Two-wire, addressable, perfect for command-byte protocol |

### 📈 Key Learning Outcomes

1. **Interrupt-safe programming** — Understanding why `delay()` inside `Wire.onReceive()` causes system hangs, and adopting flag-based state machines instead
2. **Non-blocking architecture** — Replacing every `delay()` in the main loop with `millis()`-based timing to keep all subsystems responsive
3. **Graceful degradation** — Designing systems that lose features progressively (cloud → manual → emergency-only) rather than failing completely
4. **Hardware-software co-design** — Learning that the code is only half the project; wire routing, power budgets, and component datasheets matter just as much
5. **Safety-first architecture** — Emergency code runs at the highest priority level and cannot be disabled by any user action except a physical button press

### ⏱️ Time Invested

- **Research & Planning:** ~2 weeks (datasheets, protocol analysis, cloud platform evaluation)
- **Hardware Assembly & Wiring:** ~1 week (breadboard prototyping, soldering, pin allocation)
- **Software Development:** ~3 weeks (iterative development, testing each subsystem individually)
- **Integration & Testing:** ~1 week (combined system testing, emergency simulations, edge cases)
- **Documentation:** Ongoing throughout development

### 🔮 Future Plans

- **Web Dashboard** — Local HTTP server on ESP8266 serving a real-time sensor dashboard
- **MQTT Integration** — Adding Home Assistant compatibility alongside Sinric Pro
- **OTA Updates** — Over-the-air firmware updates without physical USB connection
- **Multiple Room Support** — Extending the I2C bus to control multiple Arduino slaves
- **Mobile Push Notifications** — Gas/fire alerts sent to phone via Telegram bot or Pushover
- **Power Monitoring** — Adding current sensors to track appliance energy consumption

---

_This project represents genuine learning through building. Every component was chosen deliberately, every bug was debugged manually, and every safety feature was tested with real hardware. AI tools assisted in code implementation, but the architecture, hardware design, and safety logic are the product of hands-on engineering._
]]>
