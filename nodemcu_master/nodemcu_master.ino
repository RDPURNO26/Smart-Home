#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include <SinricProLock.h>
#include <SinricProFan.h>
#include <DHT.h>
#include <Wire.h>

// WiFi and Sinric credentials
const char* WIFI_SSID = "Zangetsu...";
const char* WIFI_PASS = "zoroisdorA9";
const char* APP_KEY = "a09d68a2-e8ed-49f8-ad53-cc60bd2d5f50";
const char* APP_SECRET = "63ed1664-d296-4358-a11e-4653a5409564-7e657a23-4d27-434d-a2aa-2e18d98b4bda";
const char* SWITCH_ID = "68d91ffe382e7b1db3a2825c"; // LED
const char* LOCK_ID = "68d9205e5009c4f120fbe837";   // Door Lock
const char* FAN_ID = "68d94568382e7b1db3a29202";    // Main Fan

#define DHT_PIN 14          // D5/GPIO14
#define BUTTON_PIN 0        // D3/GPIO0 for auto mode/gas stop button
#define MQ8_SENSOR_PIN A0   // Analog pin for MQ-8
#define FLAME_SENSOR_PIN 12 // D6/GPIO12 for flame sensor (Digital)
#define BUZZER_PIN 16       // D0/GPIO16 for buzzer
#define VENT_FAN_RELAY_PIN 13 // D7/GPIO13 for ventilation fan relay
#define DHT_TYPE DHT11
#define ARDUINO_SLAVE_ADDR 8

// Relay logic
#define RELAY_ON LOW
#define RELAY_OFF HIGH

DHT dht(DHT_PIN, DHT_TYPE);
unsigned long lastUpdate = 0;
unsigned long lastTempCheck = 0;
unsigned long lastButtonCheck = 0;
unsigned long lastGasCheck = 0;
unsigned long lastFlameCheck = 0;
unsigned long lastBuzzerToggle = 0;
unsigned long lastEmergencyCheck = 0;
#define UPDATE_INTERVAL 60000
#define TEMP_CHECK_INTERVAL 10000
#define BUTTON_CHECK_INTERVAL 20
#define GAS_CHECK_INTERVAL 100    // Faster gas detection
#define FLAME_CHECK_INTERVAL 100  // Faster flame detection
#define BUZZER_TOGGLE_INTERVAL 100
#define EMERGENCY_CHECK_INTERVAL 100

// Command definitions
#define CMD_LIGHT_ON 1
#define CMD_LIGHT_OFF 2
#define CMD_LOCK 3
#define CMD_UNLOCK 4
#define CMD_WARNING_BLINK 5
#define CMD_WRONG_PIN_BLINK 6
#define CMD_FAN_ON 7
#define CMD_FAN_OFF 8
#define CMD_FAN_SPEED 9
#define CMD_AUTO_OFF_BLINK 10
#define CMD_FIRE_ALARM 11
#define CMD_STOP_BLINK 12

bool lockState = true;
unsigned long unlockTime = 0;
unsigned long lastBlinkTime = 0;
#define AUTO_LOCK_DELAY 60000
#define WARNING_BLINK_START 50000
#define BLINK_INTERVAL 1000

int blinkCount = 0;
const int MAX_BLINKS = 10;

const String correctPin = "1234";

// Fan state variables
bool fanPowerState = false;
int currentFanSpeed = 0;
bool autoMode = true;
float currentTemperature = 0;
float currentHumidity = 0;

// Light state variable
bool currentLightState = false;

// Button variables
bool lastButtonState = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
#define DEBOUNCE_DELAY 20

// Gas detection variables
bool gasEmergency = false;
int gasThreshold = 250; // Adjusted for faster detection
int gasValue = 0;
#define GAS_AVERAGE_COUNT 5
int gasReadings[GAS_AVERAGE_COUNT];
int gasReadingIndex = 0;

// Flame detection variables
bool fireEmergency = false;
bool flameDetected = false;

// Buzzer variables
bool buzzerState = false;
unsigned long lastFireBuzzerToggle = 0;
bool fireBuzzerState = false;

// Store previous states before emergency
bool preEmergencyLightState = false;
bool preEmergencyFanState = false;
int preEmergencyFanSpeed = 3;
bool preEmergencyAutoMode = true;
bool preEmergencyLockState = true;

// Door timer variables
bool doorTimerActive = false;
unsigned long doorUnlockStartTime = 0;

// OFFLINE EMERGENCY VARIABLES
bool wifiConnected = false;
bool sinricConnected = false;
unsigned long lastConnectionCheck = 0;
#define CONNECTION_CHECK_INTERVAL 5000

// ========== FIXED FAN CONTROL FUNCTIONS ==========

void sendFanCommand() {
  Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
  
  if (fanPowerState && currentFanSpeed > 0) {
    Wire.write(CMD_FAN_SPEED);
    Wire.write(currentFanSpeed);
    Serial.printf("Fan: ON, Speed: %d\n", currentFanSpeed);
  } else {
    Wire.write(CMD_FAN_OFF);
    currentFanSpeed = 0;
    Serial.println("Fan: OFF");
  }
  
  Wire.endTransmission();
}

void setFanSpeed(int speed) {
  if (speed < 0) speed = 0;
  if (speed > 5) speed = 5;
  
  currentFanSpeed = speed;
  fanPowerState = (speed > 0);
  
  sendFanCommand();
}

// ========== OFFLINE EMERGENCY FUNCTIONS ==========

void checkConnectionStatus() {
  wifiConnected = (WiFi.status() == WL_CONNECTED);
  sinricConnected = SinricPro.isConnected();
}

void savePreEmergencyStates() {
  preEmergencyLightState = currentLightState;
  preEmergencyFanState = fanPowerState;
  preEmergencyFanSpeed = currentFanSpeed;
  preEmergencyAutoMode = autoMode;
  preEmergencyLockState = lockState;
}

void restoreAfterEmergency() {
  fanPowerState = false;
  autoMode = false;
  currentFanSpeed = 0;
  currentLightState = false;
  
  Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
  Wire.write(CMD_STOP_BLINK);
  Wire.endTransmission();
  
  Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
  Wire.write(CMD_LIGHT_OFF);
  Wire.endTransmission();
  
  sendFanCommand();
  
  digitalWrite(VENT_FAN_RELAY_PIN, RELAY_OFF);
  
  lockState = true;
  doorTimerActive = false;
  Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
  Wire.write(CMD_LOCK);
  Wire.endTransmission();
  
  if (sinricConnected) {
    SinricProSwitch &mySwitch = SinricPro[SWITCH_ID];
    mySwitch.sendPowerStateEvent(false);
    
    SinricProLock &myLock = SinricPro[LOCK_ID];
    myLock.sendLockStateEvent(true);
    
    SinricProFan &myFan = SinricPro[FAN_ID];
    myFan.sendPowerStateEvent(false);
  }
}

void startFireEmergency() {
  if (!fireEmergency) {
    savePreEmergencyStates();
    fireEmergency = true;
    
    tone(BUZZER_PIN, 1000);
    buzzerState = true;
    lastFireBuzzerToggle = millis();
    
    currentLightState = false;
    Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
    Wire.write(CMD_LIGHT_OFF);
    Wire.endTransmission();
    
    fanPowerState = false;
    currentFanSpeed = 0;
    sendFanCommand();
    
    digitalWrite(VENT_FAN_RELAY_PIN, RELAY_OFF);
    
    lockState = false;
    doorTimerActive = false;
    Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
    Wire.write(CMD_UNLOCK);
    Wire.endTransmission();
    
    Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
    Wire.write(CMD_FIRE_ALARM);
    Wire.endTransmission();
    
    if (sinricConnected) {
      SinricProSwitch &mySwitch = SinricPro[SWITCH_ID];
      mySwitch.sendPowerStateEvent(false);
      
      SinricProLock &myLock = SinricPro[LOCK_ID];
      myLock.sendLockStateEvent(false);
      
      SinricProFan &myFan = SinricPro[FAN_ID];
      myFan.sendPowerStateEvent(false);
    }
    
    Serial.println("!!! FIRE EMERGENCY ACTIVATED !!!");
    Serial.println("EVACUATE IMMEDIATELY!");
    Serial.println("SYSTEM: Operating in OFFLINE EMERGENCY MODE");
  }
}

void stopFireEmergency() {
  if (fireEmergency) {
    fireEmergency = false;
    
    noTone(BUZZER_PIN);
    buzzerState = false;
    fireBuzzerState = false;
    
    restoreAfterEmergency();
    
    Serial.println("Fire Emergency: System reset to manual mode");
  }
}

void startGasEmergency() {
  if (!gasEmergency && !fireEmergency) {
    savePreEmergencyStates();
    gasEmergency = true;
    
    tone(BUZZER_PIN, 1000);
    buzzerState = true;
    
    currentLightState = false;
    Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
    Wire.write(CMD_LIGHT_OFF);
    Wire.endTransmission();
    
    fanPowerState = false;
    currentFanSpeed = 0;
    sendFanCommand();
    
    digitalWrite(VENT_FAN_RELAY_PIN, RELAY_ON);
    
    lockState = false;
    doorTimerActive = false;
    Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
    Wire.write(CMD_UNLOCK);
    Wire.endTransmission();
    
    if (sinricConnected) {
      SinricProSwitch &mySwitch = SinricPro[SWITCH_ID];
      mySwitch.sendPowerStateEvent(false);
      
      SinricProLock &myLock = SinricPro[LOCK_ID];
      myLock.sendLockStateEvent(false);
      
      SinricProFan &myFan = SinricPro[FAN_ID];
      myFan.sendPowerStateEvent(false);
    }
    
    Serial.println("!!! GAS EMERGENCY ACTIVATED !!!");
    Serial.printf("Gas Level: %d (Threshold: %d)\n", gasValue, gasThreshold);
    Serial.println("SYSTEM: Operating in OFFLINE EMERGENCY MODE");
  }
}

void stopGasEmergency() {
  if (gasEmergency) {
    gasEmergency = false;
    
    noTone(BUZZER_PIN);
    buzzerState = false;
    
    digitalWrite(VENT_FAN_RELAY_PIN, RELAY_OFF);
    
    restoreAfterEmergency();
    
    Serial.println("Gas Emergency: System reset to manual mode");
  }
}

// ========== EMERGENCY DETECTION (ALWAYS OFFLINE) ==========

void checkGasSensor() {
  gasReadings[gasReadingIndex] = analogRead(MQ8_SENSOR_PIN);
  gasReadingIndex = (gasReadingIndex + 1) % GAS_AVERAGE_COUNT;
  
  // Calculate moving average
  int sum = 0;
  for (int i = 0; i < GAS_AVERAGE_COUNT; i++) {
    sum += gasReadings[i];
  }
  gasValue = sum / GAS_AVERAGE_COUNT;
  
  if (gasValue > gasThreshold && !gasEmergency && !fireEmergency) {
    startGasEmergency();
  }
}

void checkFlameSensor() {
  bool flameReading = digitalRead(FLAME_SENSOR_PIN);
  
  if (flameReading == LOW && !flameDetected && !fireEmergency) {
    flameDetected = true;
    startFireEmergency();
  } 
  else if (flameReading == HIGH && flameDetected) {
    flameDetected = false;
  }
}

void handleBuzzer() {
  if (fireEmergency) {
    if (millis() - lastFireBuzzerToggle >= BUZZER_TOGGLE_INTERVAL) {
      if (fireBuzzerState) {
        tone(BUZZER_PIN, 1500);
      } else {
        tone(BUZZER_PIN, 800);
      }
      fireBuzzerState = !fireBuzzerState;
      lastFireBuzzerToggle = millis();
    }
  } 
  else if (gasEmergency) {
    if (!buzzerState) {
      tone(BUZZER_PIN, 1000);
      buzzerState = true;
    }
  } else {
    if (buzzerState) {
      noTone(BUZZER_PIN);
      buzzerState = false;
      fireBuzzerState = false;
    }
  }
}

// ========== MANUAL CONTROLS (ONLINE/OFFLINE) ==========

void manualStopVentilation() {
  digitalWrite(VENT_FAN_RELAY_PIN, RELAY_OFF);
  Serial.println("Ventilation Fan manually turned OFF");
}

bool onPowerState(const String &deviceId, bool &state) {
  if (deviceId == SWITCH_ID) {
    if (gasEmergency || fireEmergency) {
      Serial.println("Cannot control light during emergency!");
      return false;
    }
    
    if (!wifiConnected) {
      Serial.println("WiFi not connected - manual control only");
    }
    
    Serial.printf("Light turned %s\r\n", state ? "on" : "off");
    currentLightState = state;
    Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
    Wire.write(state ? CMD_LIGHT_ON : CMD_LIGHT_OFF);
    Wire.endTransmission();
    return true;
  } 
  return false;
}

bool onLockState(const String &deviceId, bool &state) {
  if (deviceId == LOCK_ID) {
    if (gasEmergency || fireEmergency) {
      Serial.println("Cannot change lock state during emergency!");
      return false;
    }
    
    Serial.printf("Lock %s\r\n", state ? "locked" : "unlocked");
    
    if (state) {
      lockState = true;
      doorTimerActive = false;
      Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
      Wire.write(CMD_LOCK);
      Wire.endTransmission();
      Serial.println("Door manually locked - timer stopped");
    } else {
      lockState = false;
      doorTimerActive = true;
      doorUnlockStartTime = millis();
      unlockTime = millis();
      lastBlinkTime = 0;
      blinkCount = 0;
      Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
      Wire.write(CMD_UNLOCK);
      Wire.endTransmission();
      Serial.println("Door unlocked - 60 second timer started");
      Serial.printf("Timer started at: %lu\n", doorUnlockStartTime);
    }
    
    if (sinricConnected) {
      SinricProLock &myLock = SinricPro[LOCK_ID];
      myLock.sendLockStateEvent(lockState);
    }
    return true;
  }
  return false;
}

bool onFanPowerState(const String &deviceId, bool &state) {
  if (deviceId == FAN_ID) {
    if (gasEmergency || fireEmergency) {
      Serial.println("Cannot change fan state during emergency!");
      return false;
    }
    
    Serial.printf("Main Fan turned %s\r\n", state ? "on" : "off");
    fanPowerState = state;
    
    if (state) {
      if (autoMode) {
        adjustFanBasedOnTemperature();
      } else {
        setFanSpeed(3);
      }
    } else {
      setFanSpeed(0);
    }
    
    return true;
  }
  return false;
}

bool onPowerLevel(const String &deviceId, int &powerLevel) {
  if (deviceId == FAN_ID) {
    if (gasEmergency || fireEmergency) {
      Serial.println("Cannot change fan speed during emergency!");
      return false;
    }
    
    if (autoMode) {
      autoMode = false;
      Serial.println("Speed change detected - Switching to Manual mode");
    }
    
    Serial.printf("PowerLevel received: %d%%\r\n", powerLevel);
    
    int speedLevel;
    if (powerLevel == 0) {
      speedLevel = 0;
    } else if (powerLevel <= 20) {
      speedLevel = 1;
    } else if (powerLevel <= 40) {
      speedLevel = 2;
    } else if (powerLevel <= 60) {
      speedLevel = 3;
    } else if (powerLevel <= 80) {
      speedLevel = 4;
    } else {
      speedLevel = 5;
    }
    
    setFanSpeed(speedLevel);
    
    return true;
  }
  return false;
}

// ========== OTHER FUNCTIONS ==========

void toggleAutoMode() {
  if (gasEmergency) {
    stopGasEmergency();
    return;
  }
  if (fireEmergency) {
    stopFireEmergency();
    return;
  }
  
  autoMode = !autoMode;
  
  if (autoMode) {
    Serial.println("Auto mode ON - Fan speed controlled by temperature");
    Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
    Wire.write(CMD_AUTO_OFF_BLINK);
    Wire.endTransmission();
    
    if (fanPowerState) {
      adjustFanBasedOnTemperature();
    }
  } else {
    Serial.println("Auto mode OFF - Fan at universal speed 3");
    
    if (fanPowerState) {
      setFanSpeed(3);
    }
  }
}

void adjustFanBasedOnTemperature() {
  if (!fanPowerState || !autoMode || gasEmergency || fireEmergency) return;
  
  int newSpeed = 0;
  
  if (currentTemperature < 25.0) {
    newSpeed = 0;
  } else if (currentTemperature < 27.0) {
    newSpeed = 1;
  } else if (currentTemperature < 29.0) {
    newSpeed = 2;
  } else if (currentTemperature < 31.0) {
    newSpeed = 3;
  } else if (currentTemperature < 33.0) {
    newSpeed = 4;
  } else {
    newSpeed = 5;
  }
  
  if (newSpeed != currentFanSpeed) {
    setFanSpeed(newSpeed);
    
    if (newSpeed == 0 && sinricConnected) {
      SinricProFan &myFan = SinricPro[FAN_ID];
      myFan.sendPowerStateEvent(false);
    }
  }
}

void readTemperature() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  if (!isnan(temp) && !isnan(hum)) {
    currentTemperature = temp;
    currentHumidity = hum;
    if (autoMode && fanPowerState && !gasEmergency && !fireEmergency) {
      adjustFanBasedOnTemperature();
    }
  }
}

void handleButton() {
  bool reading = digitalRead(BUTTON_PIN);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;
      
      if (buttonState == LOW) {
        if (gasEmergency) {
          stopGasEmergency();
        } else if (fireEmergency) {
          stopFireEmergency();
        } else {
          toggleAutoMode();
        }
      }
    }
  }
  
  lastButtonState = reading;
}

void handlePinInput() {
  if (Serial.available()) {
    String pin = Serial.readStringUntil('\n');
    pin.trim();
    if (pin == correctPin) {
      Serial.println("Correct PIN! Unlocking...");
      lockState = false;
      doorTimerActive = true;
      doorUnlockStartTime = millis();
      unlockTime = millis();
      lastBlinkTime = 0;
      blinkCount = 0;
      Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
      Wire.write(CMD_UNLOCK);
      Wire.endTransmission();
      if (sinricConnected) {
        SinricProLock &myLock = SinricPro[LOCK_ID];
        myLock.sendLockStateEvent(lockState);
      }
      Serial.println("Door unlocked via PIN - 60 second timer started");
      Serial.printf("Timer started at: %lu\n", doorUnlockStartTime);
    } else {
      Serial.println("Wrong PIN!");
      Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
      Wire.write(CMD_WRONG_PIN_BLINK);
      Wire.endTransmission();
    }
  }
}

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toUpperCase();
    
    if (command == "AUTO") {
      toggleAutoMode();
    }
    else if (command.startsWith("SPEED")) {
      int speed = command.substring(5).toInt();
      if (speed >= 0 && speed <= 5) {
        autoMode = false;
        setFanSpeed(speed);
        
        if (sinricConnected) {
          SinricProFan &myFan = SinricPro[FAN_ID];
          myFan.sendPowerStateEvent(speed > 0);
        }
      }
    }
    else if (command == "STATUS") {
      Serial.printf("Light: %s, Main Fan: %s, Speed: %d, Auto: %s\n",
                   currentLightState ? "ON" : "OFF",
                   fanPowerState ? "ON" : "OFF",
                   currentFanSpeed,
                   autoMode ? "ON" : "OFF");
      Serial.printf("Vent Fan: %s, Temp: %.1f°C, Hum: %.1f%%, Gas: %d, Flame: %s\n",
                   (digitalRead(VENT_FAN_RELAY_PIN) == RELAY_ON) ? "ON" : "OFF",
                   currentTemperature,
                   currentHumidity,
                   gasValue,
                   flameDetected ? "DETECTED" : "No flame");
      Serial.printf("Gas Emergency: %s, Fire Emergency: %s\n",
                   gasEmergency ? "YES" : "NO",
                   fireEmergency ? "YES" : "NO");
      Serial.printf("WiFi: %s, SinricPro: %s\n",
                   wifiConnected ? "CONNECTED" : "DISCONNECTED",
                   sinricConnected ? "CONNECTED" : "DISCONNECTED");
      unsigned long timeLeft = 0;
      if (doorTimerActive && !lockState) {
        timeLeft = 60 - (millis() - doorUnlockStartTime) / 1000;
        if (timeLeft < 0) timeLeft = 0;
      }
      Serial.printf("Door: %s, Timer Active: %s, Time Left: %ld seconds\n",
                   lockState ? "LOCKED" : "UNLOCKED",
                   doorTimerActive ? "YES" : "NO",
                   timeLeft);
    }
    else if (command == "GAS_TEST") {
      gasValue = gasThreshold + 100;
      startGasEmergency();
    }
    else if (command == "GAS_STOP") {
      stopGasEmergency();
    }
    else if (command == "VENT_STOP") {
      manualStopVentilation();
    }
    else if (command == "FIRE_TEST") {
      startFireEmergency();
    }
    else if (command == "FIRE_STOP") {
      stopFireEmergency();
    }
    else if (command == "BUZZER_TEST") {
      tone(BUZZER_PIN, 1000);
      delay(1000);
      noTone(BUZZER_PIN);
    }
    else if (command.startsWith("THRESHOLD")) {
      int newThreshold = command.substring(9).toInt();
      if (newThreshold > 0 && newThreshold < 1024) {
        gasThreshold = newThreshold;
        Serial.printf("Gas threshold set to: %d\n", gasThreshold);
      }
    }
    else if (command == "RESET") {
      restoreAfterEmergency();
    }
    else if (command == "LIGHT_ON") {
      currentLightState = true;
      Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
      Wire.write(CMD_LIGHT_ON);
      Wire.endTransmission();
    }
    else if (command == "LIGHT_OFF") {
      currentLightState = false;
      Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
      Wire.write(CMD_LIGHT_OFF);
      Wire.endTransmission();
    }
    else if (command == "VENT_ON") {
      digitalWrite(VENT_FAN_RELAY_PIN, RELAY_ON);
    }
    else if (command == "VENT_OFF") {
      digitalWrite(VENT_FAN_RELAY_PIN, RELAY_OFF);
    }
    else if (command == "LOCK") {
      lockState = true;
      doorTimerActive = false;
      Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
      Wire.write(CMD_LOCK);
      Wire.endTransmission();
      if (sinricConnected) {
        SinricProLock &myLock = SinricPro[LOCK_ID];
        myLock.sendLockStateEvent(lockState);
      }
      Serial.println("Door manually locked");
    }
    else if (command == "UNLOCK") {
      lockState = false;
      doorTimerActive = true;
      doorUnlockStartTime = millis();
      unlockTime = millis();
      lastBlinkTime = 0;
      blinkCount = 0;
      Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
      Wire.write(CMD_UNLOCK);
      Wire.endTransmission();
      if (sinricConnected) {
        SinricProLock &myLock = SinricPro[LOCK_ID];
        myLock.sendLockStateEvent(lockState);
      }
      Serial.println("Door manually unlocked - 60 second timer started");
      Serial.printf("Timer started at: %lu\n", doorUnlockStartTime);
    }
    else if (command == "FAN_ON") {
      setFanSpeed(3);
    }
    else if (command == "FAN_OFF") {
      setFanSpeed(0);
    }
    else if (command == "NETWORK_STATUS") {
      checkConnectionStatus();
      Serial.printf("WiFi: %s, SinricPro: %s\n", 
                   wifiConnected ? "CONNECTED" : "DISCONNECTED",
                   sinricConnected ? "CONNECTED" : "DISCONNECTED");
    }
  }
}

void sendSensorData() {
  readTemperature();
  if (!isnan(currentTemperature) && !isnan(currentHumidity)) {
    Serial.printf("Temp: %.1f°C, Hum: %.1f%%, Gas: %d, Flame: %s, WiFi: %s\n", 
                  currentTemperature, currentHumidity, gasValue,
                  flameDetected ? "DETECTED" : "No flame",
                  wifiConnected ? "ONLINE" : "OFFLINE");
  }
}

void setupWiFi() {
  Serial.printf("Connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long wifiStartTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStartTime < 10000) {
    Serial.print(".");
    delay(250);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.printf("Connected! IP: %s\r\n", WiFi.localIP().toString().c_str());
    wifiConnected = true;
  } else {
    Serial.println();
    Serial.println("WiFi connection failed - Emergency systems remain ACTIVE OFFLINE");
    wifiConnected = false;
  }
}

void handleReconnect() {
  static unsigned long lastReconnectAttempt = 0;
  
  if (!wifiConnected && millis() - lastReconnectAttempt > 30000) {
    Serial.println("Attempting WiFi reconnection...");
    WiFi.disconnect();
    WiFi.reconnect();
    lastReconnectAttempt = millis();
    
    delay(5000);
    checkConnectionStatus();
    
    if (wifiConnected) {
      Serial.println("WiFi reconnected!");
    }
  }
}

void setupSinricPro() {
  SinricProSwitch &mySwitch = SinricPro[SWITCH_ID];
  mySwitch.onPowerState(onPowerState);
  
  SinricProLock &myLock = SinricPro[LOCK_ID];
  myLock.onLockState(onLockState);
  
  SinricProFan &myFan = SinricPro[FAN_ID];
  myFan.onPowerState(onFanPowerState);
  myFan.onPowerLevel(onPowerLevel);
  
  SinricPro.onConnected([]() { 
    Serial.println("✅ Connected to Sinric Pro"); 
    sinricConnected = true;
    // Sync states on connect
    SinricProSwitch &mySwitch = SinricPro[SWITCH_ID];
    mySwitch.sendPowerStateEvent(currentLightState);
    SinricProLock &myLock = SinricPro[LOCK_ID];
    myLock.sendLockStateEvent(lockState);
    SinricProFan &myFan = SinricPro[FAN_ID];
    myFan.sendPowerStateEvent(fanPowerState);
  });
  
  SinricPro.onDisconnected([]() { 
    Serial.println("❌ Disconnected from Sinric Pro"); 
    sinricConnected = false;
  });
  
  SinricPro.begin(APP_KEY, APP_SECRET);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nNodeMCU Master Starting...");
  Serial.println("=== OFFLINE EMERGENCY SYSTEM ACTIVATED ===");
  
  // Initialize gas readings array
  for (int i = 0; i < GAS_AVERAGE_COUNT; i++) {
    gasReadings[i] = 0;
  }
  
  // Setup pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(MQ8_SENSOR_PIN, INPUT);
  pinMode(VENT_FAN_RELAY_PIN, OUTPUT);
  digitalWrite(VENT_FAN_RELAY_PIN, RELAY_OFF);
  
  // Initialize I2C
  Wire.begin();
  
  dht.begin();
  
  // Preheat MQ-8 sensor
  Serial.println("Preheating MQ-8 sensor...");
  delay(60000); // 60 seconds preheat
  Serial.println("MQ-8 sensor preheated");
  
  setupWiFi();
  
  if (wifiConnected) {
    setupSinricPro();
  } else {
    Serial.println("Starting in OFFLINE MODE - Emergencies will work without internet");
  }
  
  readTemperature();
  checkConnectionStatus();
  
  setFanSpeed(0);
  
  Serial.println("Setup complete! System READY for OFFLINE emergencies.");
  Serial.println("Fire & Gas detection: 100% OFFLINE CAPABLE");
  Serial.println("Door auto-lock: 100% OFFLINE CAPABLE");
  Serial.println("Manual controls: Available via serial/button");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // ========== HIGHEST PRIORITY: OFFLINE EMERGENCY CHECKS ==========
  if (currentMillis - lastGasCheck >= GAS_CHECK_INTERVAL) {
    checkGasSensor();
    lastGasCheck = currentMillis;
  }
  
  if (currentMillis - lastFlameCheck >= FLAME_CHECK_INTERVAL) {
    checkFlameSensor();
    lastFlameCheck = currentMillis;
  }
  
  // ========== HIGH PRIORITY: OFFLINE BUZZER AND BUTTON ==========
  handleBuzzer();
  handleButton();
  
  // ========== MEDIUM PRIORITY: ONLINE COMMUNICATION ==========
  if (wifiConnected) {
    SinricPro.handle();
  }
  
  // Connection status check
  if (currentMillis - lastConnectionCheck >= CONNECTION_CHECK_INTERVAL) {
    checkConnectionStatus();
    handleReconnect();
    lastConnectionCheck = currentMillis;
  }
  
  // ========== LOW PRIORITY: USER INPUT ==========
  handlePinInput();
  handleSerialCommands();
  
  // ========== DOOR TIMER LOGIC (OFFLINE CAPABLE) ==========
  if (doorTimerActive && !lockState && !gasEmergency && !fireEmergency) {
    unsigned long now = millis();
    unsigned long timeElapsed = now - doorUnlockStartTime;
    
    static unsigned long lastDebugPrint = 0;
    if (timeElapsed % 10000 == 0 && now - lastDebugPrint >= 1000) {
      Serial.printf("Door timer: %ld seconds elapsed\n", timeElapsed / 1000);
      lastDebugPrint = now;
    }
    
    if (timeElapsed >= WARNING_BLINK_START && timeElapsed < AUTO_LOCK_DELAY && blinkCount < MAX_BLINKS) {
      if (now - lastBlinkTime >= BLINK_INTERVAL) {
        Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
        Wire.write(CMD_WARNING_BLINK);
        Wire.endTransmission();
        lastBlinkTime = now;
        blinkCount++;
        Serial.printf("Warning blink %d/%d - Door will lock in %ld seconds\n", 
                     blinkCount, MAX_BLINKS, (AUTO_LOCK_DELAY - timeElapsed) / 1000);
      }
    }
    
    if (timeElapsed >= AUTO_LOCK_DELAY) {
      lockState = true;
      doorTimerActive = false;
      Wire.beginTransmission(ARDUINO_SLAVE_ADDR);
      Wire.write(CMD_LOCK);
      Wire.endTransmission();
      if (sinricConnected) {
        SinricProLock &myLock = SinricPro[LOCK_ID];
        myLock.sendLockStateEvent(lockState);
      }
      Serial.println("60-second timer expired - Door auto-locked");
    }
  }
  
  // ========== LOWEST PRIORITY: TEMPERATURE AND UPDATES ==========
  if (currentMillis - lastTempCheck >= TEMP_CHECK_INTERVAL) {
    readTemperature();
    lastTempCheck = currentMillis;
  }
  
  if (currentMillis - lastUpdate >= UPDATE_INTERVAL) {
    sendSensorData();
    lastUpdate = currentMillis;
  }
  
  delay(5);
}