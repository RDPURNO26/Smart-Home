<![CDATA[# Serial Command Reference

All commands are sent via the Serial Monitor at **115200 baud**. Commands are case-insensitive (automatically converted to uppercase).

---

## System Status

| Command | Description |
|---------|-------------|
| `STATUS` | Display all device states, sensor readings, emergency flags, connection status, and door timer |
| `NETWORK_STATUS` | Show WiFi and Sinric Pro connection status |

---

## Light Control

| Command | Description |
|---------|-------------|
| `LIGHT_ON` | Turn the room light on |
| `LIGHT_OFF` | Turn the room light off |

> ⚠️ Blocked during gas or fire emergencies.

---

## Door Lock Control

| Command | Description |
|---------|-------------|
| `LOCK` | Lock the door immediately, stop auto-lock timer |
| `UNLOCK` | Unlock the door, start 60-second auto-lock timer |
| `1234` | Enter PIN code to unlock (default PIN: `1234`) |

### Auto-Lock Timer Behavior
1. Door unlocked → 60-second countdown starts
2. At 50 seconds → warning LED blinks on Arduino (10 blinks, 1/sec)
3. At 60 seconds → door automatically re-locks
4. Manual `LOCK` command → cancels the timer immediately

> ⚠️ Wrong PIN triggers rapid error blink pattern (20 blinks at 200ms).

---

## Fan Control

| Command | Description |
|---------|-------------|
| `FAN_ON` | Turn fan on at speed 3 |
| `FAN_OFF` | Turn fan off |
| `SPEED0` – `SPEED5` | Set specific fan speed (disables auto mode) |
| `AUTO` | Toggle temperature-based automatic fan speed |

### Speed Levels

| Speed | PWM Value | Duty Cycle | Condition (Auto Mode) |
|-------|-----------|------------|----------------------|
| 0 | 0 | Off | < 25°C |
| 1 | 100 | ~39% | 25–27°C |
| 2 | 128 | ~50% | 27–29°C |
| 3 | 153 | ~60% | 29–31°C |
| 4 | 204 | ~80% | 31–33°C |
| 5 | 255 | 100% | > 33°C |

> 💡 Changing speed via `SPEEDx` command automatically switches to manual mode.

---

## Ventilation Fan (Relay)

| Command | Description |
|---------|-------------|
| `VENT_ON` | Manually turn on the ventilation/exhaust fan |
| `VENT_OFF` | Manually turn off the ventilation fan |
| `VENT_STOP` | Stop ventilation (alias during gas emergency) |

> 💡 The ventilation fan is automatically activated during gas emergencies and deactivated during fire emergencies.

---

## Emergency Commands

| Command | Description |
|---------|-------------|
| `GAS_TEST` | Simulate a gas emergency (for testing) |
| `GAS_STOP` | Clear gas emergency, reset system to manual |
| `FIRE_TEST` | Simulate a fire emergency (for testing) |
| `FIRE_STOP` | Clear fire emergency, reset system to manual |
| `BUZZER_TEST` | Sound buzzer for 1 second (diagnostic) |
| `RESET` | Force-reset all systems to default safe state |

### Emergency Protocols

**Gas Emergency:**
- Buzzer: steady 1000Hz tone
- Lights: OFF
- Main fan: OFF
- Ventilation fan: ON (exhaust the gas)
- Door: UNLOCKED (evacuation)
- LED: rapid blink on Arduino

**Fire Emergency:**
- Buzzer: two-tone siren (800Hz / 1500Hz alternating at 100ms)
- Lights: OFF
- Main fan: OFF
- Ventilation fan: OFF (don't feed the fire)
- Door: UNLOCKED (evacuation)
- LED: rapid blink on Arduino (200ms)

---

## Sensor Configuration

| Command | Description |
|---------|-------------|
| `THRESHOLD250` | Set gas detection threshold to 250 (default) |
| `THRESHOLDxxx` | Set custom threshold (1–1023) |

> 💡 Lower threshold = more sensitive. Adjust based on your environment's baseline readings. Use `STATUS` to see current gas levels.

---

## Physical Button (GPIO0 / D3)

The onboard button serves multiple functions depending on system state:

| System State | Button Action |
|---|---|
| Normal operation | Toggle auto fan mode ON/OFF |
| Gas emergency active | Stop gas emergency, reset system |
| Fire emergency active | Stop fire emergency, reset system |

---

## Example Session

```
NodeMCU Master Starting...
=== OFFLINE EMERGENCY SYSTEM ACTIVATED ===
Preheating MQ-8 sensor...
MQ-8 sensor preheated
Connecting to WiFi... Connected! IP: 192.168.1.42
✅ Connected to Sinric Pro
Setup complete! System READY for OFFLINE emergencies.

> STATUS
Light: OFF, Main Fan: OFF, Speed: 0, Auto: ON
Vent Fan: OFF, Temp: 28.3°C, Hum: 65.0%, Gas: 42, Flame: No flame
Gas Emergency: NO, Fire Emergency: NO
WiFi: CONNECTED, SinricPro: CONNECTED
Door: LOCKED, Timer Active: NO, Time Left: 0 seconds

> 1234
Correct PIN! Unlocking...
Door unlocked via PIN - 60 second timer started

> AUTO
Auto mode OFF - Fan at universal speed 3

> SPEED5
Fan Speed Set: 5

> GAS_TEST
!!! GAS EMERGENCY ACTIVATED !!!
Gas Level: 350 (Threshold: 250)
SYSTEM: Operating in OFFLINE EMERGENCY MODE

> GAS_STOP
Gas Emergency: System reset to manual mode
```
]]>
