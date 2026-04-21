<![CDATA[# System Architecture

Deep-dive into the Smart Home IoT System's internal design, communication protocol, and execution model.

---

## Overview

The system follows a **master-slave architecture** with strict separation of concerns:

- **NodeMCU ESP8266 (Master)** → All intelligence: sensing, decision-making, networking, state management
- **Arduino Uno (Slave)** → All actuation: motors, relays, LEDs, physical output

They communicate over a **single I2C bus** using a lightweight byte-command protocol.

---

## I2C Command Protocol

The master sends 1–2 byte commands to the slave at address `0x08`:

| Command Byte | Value | Extra Byte | Action |
|---|---|---|---|
| `CMD_LIGHT_ON` | 1 | — | Turn light relay ON |
| `CMD_LIGHT_OFF` | 2 | — | Turn light relay OFF |
| `CMD_LOCK` | 3 | — | Move servo to 0° (locked) |
| `CMD_UNLOCK` | 4 | — | Move servo to 180° (unlocked) |
| `CMD_WARNING_BLINK` | 5 | — | Start slow warning blink (500ms, 10 cycles) |
| `CMD_WRONG_PIN_BLINK` | 6 | — | Start fast error blink (200ms, 20 cycles) |
| `CMD_FAN_ON` | 7 | — | Fan on at speed 3 |
| `CMD_FAN_OFF` | 8 | — | Fan off |
| `CMD_FAN_SPEED` | 9 | Speed (0–5) | Set specific fan speed |
| `CMD_AUTO_OFF_BLINK` | 10 | — | Short confirmation blink (300ms, 4 cycles) |
| `CMD_FIRE_ALARM` | 11 | — | Full fire protocol on slave |
| `CMD_STOP_BLINK` | 12 | — | Stop all blinking immediately |

### Design Decisions
- **No acknowledgment** — Commands are fire-and-forget. The master trusts the slave to execute. ACK would add latency to the emergency pipeline.
- **Single-byte commands** — Keep I2C transmissions as short as possible to avoid bus contention.
- **`CMD_FAN_SPEED` is the only 2-byte command** — Speed level is sent as the second byte in the same transmission.

---

## Main Loop Execution Model

The NodeMCU runs a **priority-based cooperative scheduler** in `loop()`. No RTOS, no interrupts for timing — pure `millis()`-based polling:

```
Priority 0 (CRITICAL):  Gas sensor check        — every 100ms
                         Flame sensor check      — every 100ms

Priority 1 (HIGH):      Buzzer handler           — every loop iteration
                         Physical button handler  — every loop iteration

Priority 2 (MEDIUM):    SinricPro.handle()       — every loop (if WiFi connected)
                         Connection status check  — every 5 seconds
                         WiFi reconnection        — every 30 seconds

Priority 3 (LOW):       Serial command handler   — every loop iteration
                         PIN input handler        — every loop iteration

Priority 4 (TIMER):     Door auto-lock timer     — continuous tracking
                         Warning blink trigger    — at 50s elapsed

Priority 5 (PERIODIC):  Temperature reading      — every 10 seconds
                         Sensor data broadcast    — every 60 seconds

Yield:                   delay(5)                 — allow WiFi stack processing
```

### Why This Order?
- Gas and flame detection **must never be starved**. Even if WiFi is reconnecting (which blocks for 5 seconds), the emergency checks run first.
- The buzzer and button are checked every loop because emergency stop must be instant.
- `SinricPro.handle()` is skipped entirely if WiFi is down — no exceptions thrown, no hangs.
- The 5ms `delay()` at the end is required by the ESP8266 to service its WiFi stack internally.

---

## State Machine

The system has four operational modes:

```
                    ┌──────────────┐
        ┌──────────▶│   NORMAL     │◀──────────┐
        │           │              │            │
        │           │ All controls │            │
        │           │   available  │            │
        │           └──────┬───────┘            │
        │                  │                    │
        │       Gas detected │  Flame detected  │
        │                  │                    │
        │                  ▼                    │
        │     ┌────────────────────────┐        │
 Button │     │    GAS EMERGENCY       │        │ Button
  press │     │                        │        │  press
        │     │ • Buzzer: steady tone  │        │
        │     │ • Vent fan: ON         │        │
        │     │ • All else: OFF/UNLOCK │        │
        │     └────────────────────────┘        │
        │                                       │
        │     ┌────────────────────────┐        │
        │     │    FIRE EMERGENCY      │        │
        └─────│                        │────────┘
              │ • Buzzer: two-tone     │
              │ • Vent fan: OFF        │
              │ • All else: OFF/UNLOCK │
              └────────────────────────┘
```

### State Transition Rules
- **Gas → Fire:** If flame is detected during a gas emergency, fire protocol takes over (fire has higher priority)
- **Emergency → Normal:** Only via physical button press or serial `GAS_STOP`/`FIRE_STOP` command
- **No user control during emergency:** All Sinric Pro / voice / serial device commands are rejected with "Cannot control during emergency!" message

---

## Arduino Slave — Blink State Machine

The slave manages four independent blink patterns, with strict priority:

```
Fire Alarm (fastest, indefinite)
    ↓ (if not active)
Wrong PIN (fast, 20 cycles)
    ↓ (if not active)
Warning (slow, 10 cycles)
    ↓ (if not active)
Auto Mode Confirm (medium, 4 cycles)
    ↓ (if not active)
LED OFF (idle)
```

- Each pattern is **non-blocking** — uses `millis()` comparison, never `delay()`
- `CMD_STOP_BLINK` cancels all patterns immediately
- Starting a new pattern calls `stopAllBlinking()` first (except fire alarm, which takes absolute priority)
- The fire alarm blink runs indefinitely until explicitly stopped

### Fan Kick-Start Logic

```cpp
if (speedLevel > 0 && currentFanSpeed == 0) {
    analogWrite(FAN_PWM_PIN, 255);  // Full power
    delay(500);                      // 500ms burst
}
// Then drop to target PWM value
```

This is the **only `delay()` in the slave's code**. It's acceptable because it only occurs on cold-start transitions (fan was off, now turning on) and the 500ms is needed to overcome the motor's static friction at lower PWM values.

---

## WiFi Resilience

```
Boot
 │
 ├─ WiFi connects within 10s? ──▶ YES: Setup Sinric Pro, full online mode
 │                                 │
 │                                 ├─ Sinric Pro connects? ──▶ Sync all states
 │                                 │
 │                                 └─ Sinric disconnects later ──▶ Mark offline,
 │                                                                  keep WiFi
 │
 └─ WiFi fails to connect? ──▶ Start in OFFLINE MODE
                                 │
                                 └─ Every 30s: attempt reconnection
                                      │
                                      └─ If reconnected: setup Sinric Pro
```

- State variables `wifiConnected` and `sinricConnected` are checked before any cloud operation
- `SinricPro.handle()` is called only when WiFi is confirmed connected
- On reconnection, a full state sync pushes current light/lock/fan states to the cloud

---

## Memory & Resource Usage

| Resource | NodeMCU | Arduino |
|---|---|---|
| Flash | ~350KB / 4MB | ~7KB / 32KB |
| RAM | ~35KB / 80KB | ~1.2KB / 2KB |
| I2C Address | Master (no address) | Slave @ 0x08 |
| Timers Used | None modified | Timer0 (PWM prescaler) |
| Interrupts | Wire ISR (I2C) | Wire ISR (I2C receive) |
| WiFi Stack | ESP8266 RTOS internal | N/A |
]]>
