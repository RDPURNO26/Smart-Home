<![CDATA[<p align="center">
  <img src="docs/assets/smart-home-banner.svg" alt="Smart Home IoT System" width="800"/>
</p>

<h1 align="center">🏠 Smart Home IoT System</h1>

<p align="center">
  <strong>A dual-microcontroller smart home automation system with real-time safety monitoring, voice control via Alexa/Google Home, and fully offline emergency response.</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Platform-ESP8266%20%2B%20Arduino-blue?style=flat-square" alt="Platform"/>
  <img src="https://img.shields.io/badge/Cloud-Sinric%20Pro-purple?style=flat-square" alt="Cloud"/>
  <img src="https://img.shields.io/badge/Voice-Alexa%20%7C%20Google%20Home-orange?style=flat-square" alt="Voice"/>
  <img src="https://img.shields.io/badge/Safety-Gas%20%2B%20Fire%20Detection-red?style=flat-square" alt="Safety"/>
  <img src="https://img.shields.io/badge/License-MIT-green?style=flat-square" alt="License"/>
</p>

---

## 🔍 What This Does

This project turns a regular room into an intelligent, safety-aware living space. A **NodeMCU ESP8266** acts as the brain (master), coordinating with an **Arduino Uno** (slave) over I2C to control real hardware — lights, door locks, fans, and environmental sensors.

**The key innovation:** the system operates a **priority-based emergency pipeline** that works _entirely offline_. If your WiFi goes down and a gas leak or fire is detected, the system still unlocks doors, kills power to appliances, activates the buzzer, and ventilates the room — without needing any cloud connection.

You can control everything through:
- 🗣️ **Alexa / Google Home** voice commands (via Sinric Pro)
- 📱 **Sinric Pro mobile app** from anywhere in the world
- ⌨️ **Serial terminal** commands for debugging
- 🔘 **Physical button** on the device (auto-mode toggle + emergency reset)
- 🔑 **PIN code** entry via serial for door access

---

## ✨ Features

### 🏠 Home Automation
- **Smart Light Control** — On/off via voice, app, or serial
- **Servo Door Lock** — PIN-protected with 60-second auto-lock timer and LED warning blinks before re-locking
- **5-Speed PWM Fan** — Manual speed control (1–5) or automatic temperature-based adjustment
- **Ventilation Fan Relay** — Dedicated exhaust fan triggered automatically during gas emergencies

### 🔥 Safety Systems (100% Offline Capable)
- **MQ-8 Hydrogen/Gas Sensor** — Moving average detection with configurable threshold, triggers gas emergency protocol
- **Flame Sensor** — Digital IR flame detection, triggers fire emergency protocol instantly
- **Piezo Buzzer Alarm** — Distinct patterns: two-tone siren for fire, steady tone for gas
- **Emergency Protocol:**
  1. Cuts power to lights and main fan
  2. Unlocks all doors for evacuation
  3. Activates ventilation fan (gas) or disables it (fire)
  4. Triggers rapid LED blinking on Arduino
  5. Sounds buzzer alarm
  6. Syncs state to cloud if online

### 🌡️ Environmental Monitoring
- **DHT11 Temperature & Humidity** — 10-second polling, drives auto fan speed
- **Auto Fan Mode** — Temperature-mapped 6-tier speed curve (off at <25°C, max at >33°C)
- **Gas Level Monitoring** — Continuous analog readings with sliding-window averaging

### 🔗 Connectivity
- **Sinric Pro Integration** — Real-time device control from anywhere
- **Auto WiFi Reconnection** — 30-second retry loop, graceful degradation to offline mode
- **State Sync on Reconnect** — All device states pushed to cloud on connection restore

---

## 🛠️ Tech Stack

| Category | Technology |
|---|---|
| **Master MCU** | NodeMCU ESP8266 (ESP-12E) |
| **Slave MCU** | Arduino Uno (ATmega328P) |
| **Communication** | I2C (Wire library) @ address `0x08` |
| **Cloud Platform** | [Sinric Pro](https://sinric.pro/) |
| **Voice Assistants** | Amazon Alexa, Google Home (via Sinric Pro) |
| **Temperature Sensor** | DHT11 |
| **Gas Sensor** | MQ-8 (Hydrogen) — analog read with moving average |
| **Flame Sensor** | IR Flame Detector — digital (active LOW) |
| **Door Lock** | SG90 Servo Motor (0° locked / 180° unlocked) |
| **Fan Control** | PWM @ ~31.25 kHz with kick-start pulse |
| **Relay Module** | Active-LOW relay for ventilation fan |
| **Buzzer** | Active piezo buzzer (tone/noTone) |
| **Language** | C++ (Arduino framework) |
| **IDE** | Arduino IDE / Arduino Cloud Editor |

---

## 📂 Project Structure

```
Smart Home/
├── README.md                       # This file
├── ABOUT.md                        # Project journey and technical decisions
├── LICENSE                         # MIT License
├── .gitignore                      # Ignores secrets and build artifacts
│
├── src/
│   ├── nodemcu_master/             # ESP8266 master controller
│   │   └── nodemcu_master.ino      # Main orchestrator (986 lines)
│   │
│   └── arduino_slave/              # Arduino Uno slave controller
│       └── arduino_slave.ino       # Hardware actuator (281 lines)
│
├── config/
│   ├── secrets.example.h           # Template for WiFi & API credentials
│   └── thingProperties.example.h   # Template for Arduino IoT Cloud config
│
└── docs/
    ├── WIRING.md                   # Complete wiring diagram & pin mapping
    ├── COMMANDS.md                 # Serial command reference
    ├── ARCHITECTURE.md             # System architecture deep-dive
    └── assets/                     # Documentation images
        └── smart-home-banner.svg   # Repo banner
```

---

## 🚀 Installation & Setup

### Prerequisites

- [Arduino IDE](https://www.arduino.cc/en/software) (2.x recommended) or PlatformIO
- NodeMCU ESP8266 board package installed
- Arduino Uno drivers
- USB cables for both boards

### Step 1: Clone the Repository

```bash
git clone https://github.com/YourUsername/smart-home-iot.git
cd smart-home-iot
```

### Step 2: Install Arduino Libraries

Open Arduino IDE → **Sketch → Include Library → Manage Libraries**, and install:

| Library | Version | Used For |
|---|---|---|
| `ESP8266WiFi` | (bundled) | WiFi connectivity |
| `SinricPro` | 3.x | Cloud + voice control |
| `SinricProSwitch` | 3.x | Light control device |
| `SinricProLock` | 3.x | Door lock device |
| `SinricProFan` | 3.x | Fan control device |
| `DHT sensor library` | Latest | Temperature & humidity |
| `Wire` | (bundled) | I2C communication |
| `Servo` | (bundled) | Door lock servo |

### Step 3: Configure Credentials

```bash
# Copy the example config and fill in your actual credentials
cp config/secrets.example.h src/nodemcu_master/secrets.h
```

Edit `src/nodemcu_master/secrets.h`:
```cpp
#define SECRET_SSID         "YourWiFiName"
#define SECRET_OPTIONAL_PASS "YourWiFiPassword"
#define SECRET_DEVICE_KEY   "your-sinric-device-key"
```

Then update the constants at the top of `nodemcu_master.ino`:
```cpp
const char* WIFI_SSID    = "YourWiFiName";
const char* WIFI_PASS    = "YourWiFiPassword";
const char* APP_KEY      = "your-sinric-app-key";
const char* APP_SECRET   = "your-sinric-app-secret";
const char* SWITCH_ID    = "your-switch-device-id";
const char* LOCK_ID      = "your-lock-device-id";
const char* FAN_ID       = "your-fan-device-id";
```

> ⚠️ **Never commit real credentials.** The `.gitignore` excludes `secrets.h` and `thingProperties.h`.

### Step 4: Set Up Sinric Pro

1. Create a free account at [sinric.pro](https://sinric.pro/)
2. Add three devices:
   - **Switch** (for the room light)
   - **Lock** (for the door servo)
   - **Fan** (for the main ceiling fan)
3. Copy each device's ID into your code
4. Link Sinric Pro to your Alexa or Google Home app

### Step 5: Upload Firmware

1. **Arduino Slave:** Select `Arduino Uno` board → Upload `src/arduino_slave/arduino_slave.ino`
2. **NodeMCU Master:** Select `NodeMCU 1.0 (ESP-12E Module)` board → Upload `src/nodemcu_master/nodemcu_master.ino`

### Step 6: Wire Everything

See **[docs/WIRING.md](docs/WIRING.md)** for the complete pin-by-pin wiring guide.

---

## ⚙️ Configuration

### Environment Variables / Constants

All configuration is done via `#define` constants and `const char*` variables at the top of each `.ino` file:

| Variable | File | Description |
|---|---|---|
| `WIFI_SSID` / `WIFI_PASS` | `nodemcu_master.ino` | WiFi credentials |
| `APP_KEY` / `APP_SECRET` | `nodemcu_master.ino` | Sinric Pro authentication |
| `SWITCH_ID` / `LOCK_ID` / `FAN_ID` | `nodemcu_master.ino` | Sinric device IDs |
| `gasThreshold` | `nodemcu_master.ino` | Gas detection sensitivity (default: 250) |
| `correctPin` | `nodemcu_master.ino` | Door unlock PIN code (default: "1234") |
| `AUTO_LOCK_DELAY` | `nodemcu_master.ino` | Auto-lock timer duration in ms (default: 60000) |

### Auto Fan Temperature Curve

| Temperature | Fan Speed |
|---|---|
| < 25°C | Off (0) |
| 25–27°C | Speed 1 |
| 27–29°C | Speed 2 |
| 29–31°C | Speed 3 |
| 31–33°C | Speed 4 |
| > 33°C | Speed 5 (max) |

---

## 💡 Usage Examples

### Voice Control
```
"Alexa, turn on the light"
"Hey Google, lock the door"
"Alexa, set fan speed to 80 percent"
```

### Serial Terminal Commands
Open Serial Monitor at **115200 baud**:

```
STATUS          → Show all device states, sensor readings, connection status
LIGHT_ON        → Turn on room light
LIGHT_OFF       → Turn off room light
LOCK            → Lock the door
UNLOCK          → Unlock door (starts 60s auto-lock timer)
FAN_ON          → Fan on at speed 3
FAN_OFF         → Fan off
SPEED3          → Set fan to speed 3 (auto-mode off)
AUTO            → Toggle auto temperature-based fan mode
VENT_ON         → Manually activate ventilation fan
VENT_OFF        → Manually deactivate ventilation fan
GAS_TEST        → Simulate gas emergency
GAS_STOP        → Clear gas emergency
FIRE_TEST       → Simulate fire emergency  
FIRE_STOP       → Clear fire emergency
THRESHOLD250    → Set gas detection threshold to 250
RESET           → Reset all systems to default state
1234            → Enter PIN to unlock door
```

---

## 🏗️ Architecture Overview

```
┌──────────────────────────────────────────────────────────────┐
│                     CLOUD LAYER                              │
│  ┌─────────────┐  ┌──────────┐  ┌─────────────────────┐     │
│  │ Sinric Pro   │  │  Alexa   │  │  Google Home         │     │
│  └──────┬──────┘  └────┬─────┘  └──────────┬──────────┘     │
│         └──────────────┼───────────────────┘                 │
└────────────────────────┼─────────────────────────────────────┘
                         │ WebSocket
┌────────────────────────┼─────────────────────────────────────┐
│              NodeMCU ESP8266 (MASTER)                         │
│  ┌─────────────────────┴────────────────────────────────┐    │
│  │  Priority-Based Main Loop                             │    │
│  │  ┌─────────────────────────────────────────────────┐  │    │
│  │  │ P0: Gas Sensor (100ms) + Flame Sensor (100ms)   │  │    │
│  │  │ P1: Buzzer Handler + Physical Button             │  │    │
│  │  │ P2: SinricPro.handle() (if WiFi connected)      │  │    │
│  │  │ P3: Serial Commands + PIN Input                  │  │    │
│  │  │ P4: Door Auto-Lock Timer                         │  │    │
│  │  │ P5: Temperature Polling (10s) + Data Upload (60s)│  │    │
│  │  └─────────────────────────────────────────────────┘  │    │
│  └───────────────────────────────────────────────────────┘    │
│         │ I2C Bus (SDA/SCL)                                   │
└─────────┼────────────────────────────────────────────────────┘
          │
┌─────────┼────────────────────────────────────────────────────┐
│         │        Arduino Uno (SLAVE) @ I2C Addr 0x08         │
│  ┌──────┴───────────────────────────────────────────────┐    │
│  │  Command-Driven Actuator Layer                        │    │
│  │  • Light relay (Pin 9)                                │    │
│  │  • Servo lock (Pin 10)                                │    │
│  │  • PWM fan (Pin 5, 31.25kHz)                          │    │
│  │  • Status LED (Pin 6) — warning/error/fire patterns   │    │
│  └──────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────┘
```

The system uses a **master-slave architecture** where:
- **NodeMCU** handles all sensing, logic, networking, and decision-making
- **Arduino** handles all physical actuation (motors, relays, LEDs) based on I2C byte commands
- **Emergency systems** operate at the highest loop priority and require zero internet

---

## 📚 Dependencies & Credits

### Hardware Libraries
- [ESP8266 Arduino Core](https://github.com/esp8266/Arduino) — WiFi and board support
- [SinricPro Library](https://github.com/sinricpro/esp8266-esp32-sdk) — Cloud/voice integration
- [DHT Sensor Library](https://github.com/adafruit/DHT-sensor-library) — Temperature & humidity
- Arduino built-in: `Wire.h`, `Servo.h`

### Cloud Services
- [Sinric Pro](https://sinric.pro/) — Free-tier IoT cloud platform bridging to Alexa/Google Home

### Research & Learning Resources
- [Random Nerd Tutorials](https://randomnerdtutorials.com/) — ESP8266 guides
- [Last Minute Engineers](https://lastminuteengineers.com/) — Sensor integration tutorials
- [Arduino Official Docs](https://docs.arduino.cc/) — Library references
- [Sinric Pro Documentation](https://help.sinric.pro/) — Device setup guides

---

## 🌱 Project Origin

This project was born out of genuine curiosity about how smart homes actually work at the hardware level — not just software abstractions, but real circuits, real sensors, and real safety concerns.

- **Conceived and designed** from original ideas and extensive research into IoT safety systems
- **Code implementation** assisted by AI tools, with every line fully reviewed, tested on real hardware, and integrated manually
- **Hardware selection, wiring design, and safety logic** developed through hands-on experimentation over weeks of iterative prototyping
- **Emergency protocol design** informed by real building safety standards and fire evacuation principles

This is not a copy-paste tutorial project. The offline emergency pipeline, priority-based loop architecture, and dual-MCU I2C command protocol were all custom-designed solutions to real problems encountered during development.

---

## 📄 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

---

## 👤 Author

**Purno** — [@purno](https://github.com/purno)

> Built with solder, code, and a lot of late nights. 🔧
]]>
