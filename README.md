<![CDATA[<p align="center">
  <img src="docs/assets/smart-home-banner.svg" alt="Smart Home IoT System" width="800"/>
</p>

<h1 align="center">🏠 Smart Home Automation & Safety System</h1>

<p align="center">
  <strong>An advanced dual-microcontroller smart home system featuring real-time offline safety monitoring and global voice control.</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Platform-ESP8266%20%2B%20Arduino-blue?style=flat-square" alt="Platform"/>
  <img src="https://img.shields.io/badge/Cloud-Sinric%20Pro-purple?style=flat-square" alt="Cloud"/>
  <img src="https://img.shields.io/badge/Safety-Offline%20Gas%20%2B%20Fire%20Detection-red?style=flat-square" alt="Safety"/>
  <img src="https://img.shields.io/badge/License-MIT-green?style=flat-square" alt="License"/>
</p>

---

## 📖 Overview

Most commercial smart homes become functionally useless the moment your WiFi goes down. This project solves that. 

By combining a **NodeMCU ESP8266** (the brain) with an **Arduino Uno** (the brawn), this system delivers global cloud control through Alexa/Google Home while maintaining a **100% offline emergency safety pipeline**. If a fire or gas leak occurs while your internet is disconnected, the system still unlocks doors, kills appliance power, sounds the alarms, and ventilates the room.

---

## 🔥 Key Features

### 🛡️ Bulletproof Safety (Offline)
* **Instant Flame Detection:** Digital IR sensors trigger immediate fire protocols.
* **Smart Gas Monitoring:** MQ-8 sensor with moving-average filtering detects dangerous gas buildups.
* **Offline Emergency Protocols:** Automatically cuts power, unlocks evacuation routes, and triggers sirens—**no internet required**.

### 📱 Global Control (Online)
* **Sinric Pro Integration:** Control everything from your smartphone anywhere in the world.
* **Voice Assistant Ready:** Natively supports "Hey Google" and "Alexa" commands.
* **Auto-Reconnect:** Gracefully drops to offline mode during outages and seamlessly re-syncs state when WiFi returns.

### ⚙️ Smart Automation
* **Climate Control:** Temperature-mapped 5-speed fan curve adjusts automatically.
* **Secure Access:** Servo-driven door lock with PIN pad, 60-second auto-lock timers, and visual LED warnings.

---

## 🛠️ Project Structure & Documentation

To keep the main repository clean, all technical hardware deep-dives have been moved into dedicated documentation files.

* 🔌 **[Wiring & Hardware Guide](docs/WIRING.md)** - Step-by-step pin connections, power architecture, and required components.
* 💻 **[Serial Commands](docs/COMMANDS.md)** - Complete reference for all terminal commands (e.g., `STATUS`, `LIGHT_ON`, `SPEED5`).
* 🧠 **[Architecture Deep-Dive](docs/ARCHITECTURE.md)** - Learn how the I2C master/slave protocol and the priority-based `loop()` scheduler work under the hood.
* 🚀 **[Project Journey](ABOUT.md)** - Decisions, challenges, and the story of building this system.

---

## 🚀 Quick Start Setup

### 1. Hardware & Libraries
Ensure you have the required libraries installed in the Arduino IDE:
* `SinricPro` (v3.x)
* `DHT sensor library`
* *Standard dependencies (Wire, Servo, ESP8266WiFi)*

### 2. Configure Your Credentials
1. Copy the template: `cp config/secrets.example.h src/nodemcu_master/secrets.h`
2. Open `secrets.h` and add your WiFi name, password, and your Sinric Pro API keys.
3. *Note: `.gitignore` protects this file from ever being uploaded to GitHub.*

### 3. Flash the Boards
* **Master:** Upload `src/nodemcu_master/nodemcu_master.ino` to your NodeMCU ESP8266.
* **Slave:** Upload `src/arduino_slave/arduino_slave.ino` to your Arduino Uno.

*(Refer to the [Wiring Guide](docs/WIRING.md) for connections!)*

---

## 🤝 Contributing & Origin

This project was built from the ground up as a hands-on exploration of reliable, offline-capable IoT safety architectures. 

**Author:** [RDPURNO26](https://github.com/RDPURNO26)  
**License:** [MIT License](LICENSE)
]]>
