#include <Wire.h>
#include <Servo.h>

#define ARDUINO_SLAVE_ADDR 8
#define LIGHT_PIN 9
#define SERVO_PIN 10
#define DOOR_LED_PIN 6
#define FAN_PWM_PIN 5

const int LOCK_ANGLE = 0;
const int UNLOCK_ANGLE = 180;

Servo doorLockServo;

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

int currentFanSpeed = 0;
bool fireAlarmActive = false;
unsigned long lastFireBlink = 0;
bool fireBlinkState = false;

bool warningBlinkActive = false;
unsigned long lastWarningBlink = 0;
bool warningBlinkState = false;
int warningBlinksCount = 0;

bool wrongPinBlinkActive = false;
unsigned long lastWrongPinBlink = 0;
bool wrongPinBlinkState = false;
int wrongPinBlinksCount = 0;

bool autoModeBlinkActive = false;
unsigned long lastAutoModeBlink = 0;
bool autoModeBlinkState = false;
int autoModeBlinksCount = 0;

void setup() {
  Wire.begin(ARDUINO_SLAVE_ADDR);
  Wire.onReceive(receiveEvent);
  
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, LOW);
  
  pinMode(DOOR_LED_PIN, OUTPUT);
  digitalWrite(DOOR_LED_PIN, LOW);
  
  pinMode(FAN_PWM_PIN, OUTPUT);
  analogWrite(FAN_PWM_PIN, 0);
  
  // Set PWM frequency for FAN_PWM_PIN (pin 5) to ~25kHz
  TCCR0B = TCCR0B & 0b11111000 | 0x01; // Set prescaler to 1 for ~31.25kHz on pin 5
  
  doorLockServo.attach(SERVO_PIN);
  doorLockServo.write(LOCK_ANGLE);
  
  Serial.begin(115200);
  Serial.println("Arduino Slave Started - FAN OPTIMIZED VERSION");
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (fireAlarmActive) {
    if (currentMillis - lastFireBlink >= 200) {
      fireBlinkState = !fireBlinkState;
      digitalWrite(DOOR_LED_PIN, fireBlinkState ? HIGH : LOW);
      lastFireBlink = currentMillis;
    }
    return;
  }
  
  if (wrongPinBlinkActive) {
    if (currentMillis - lastWrongPinBlink >= 200) {
      wrongPinBlinkState = !wrongPinBlinkState;
      digitalWrite(DOOR_LED_PIN, wrongPinBlinkState ? HIGH : LOW);
      lastWrongPinBlink = currentMillis;
      wrongPinBlinksCount++;
      
      if (wrongPinBlinksCount >= 20) {
        wrongPinBlinkActive = false;
        digitalWrite(DOOR_LED_PIN, LOW);
      }
    }
    return;
  }
  
  if (warningBlinkActive) {
    if (currentMillis - lastWarningBlink >= 500) {
      warningBlinkState = !warningBlinkState;
      digitalWrite(DOOR_LED_PIN, warningBlinkState ? HIGH : LOW);
      lastWarningBlink = currentMillis;
      warningBlinksCount++;
      
      if (warningBlinksCount >= 10) {
        warningBlinkActive = false;
        digitalWrite(DOOR_LED_PIN, LOW);
      }
    }
    return;
  }
  
  if (autoModeBlinkActive) {
    if (currentMillis - lastAutoModeBlink >= 300) {
      autoModeBlinkState = !autoModeBlinkState;
      digitalWrite(DOOR_LED_PIN, autoModeBlinkState ? HIGH : LOW);
      lastAutoModeBlink = currentMillis;
      autoModeBlinksCount++;
      
      if (autoModeBlinksCount >= 4) {
        autoModeBlinkActive = false;
        digitalWrite(DOOR_LED_PIN, LOW);
      }
    }
    return;
  }
}

void stopAllBlinking() {
  fireAlarmActive = false;
  warningBlinkActive = false;
  wrongPinBlinkActive = false;
  autoModeBlinkActive = false;
  digitalWrite(DOOR_LED_PIN, LOW);
}

void startAutoModeBlink() {
  stopAllBlinking();
  autoModeBlinkActive = true;
  autoModeBlinksCount = 0;
  lastAutoModeBlink = millis();
  autoModeBlinkState = true;
  digitalWrite(DOOR_LED_PIN, HIGH);
}

void startWarningBlink() {
  if (!fireAlarmActive && !wrongPinBlinkActive) {
    stopAllBlinking();
    warningBlinkActive = true;
    warningBlinksCount = 0;
    lastWarningBlink = millis();
    warningBlinkState = true;
    digitalWrite(DOOR_LED_PIN, HIGH);
  }
}

void startWrongPinBlink() {
  stopAllBlinking();
  wrongPinBlinkActive = true;
  wrongPinBlinksCount = 0;
  lastWrongPinBlink = millis();
  wrongPinBlinkState = true;
  digitalWrite(DOOR_LED_PIN, HIGH);
}

void fireAlarm() {
  stopAllBlinking();
  fireAlarmActive = true;
  
  digitalWrite(LIGHT_PIN, LOW);
  analogWrite(FAN_PWM_PIN, 0);
  currentFanSpeed = 0;
  
  doorLockServo.write(UNLOCK_ANGLE);
  
  lastFireBlink = millis();
  fireBlinkState = true;
  digitalWrite(DOOR_LED_PIN, HIGH);
  
  Serial.println("FIRE ALARM ACTIVATED ON ARDUINO!");
}

void setFanSpeed(byte speedLevel) {
  if (speedLevel == currentFanSpeed) return;
  
  if (speedLevel > 0 && currentFanSpeed == 0) {
    // Kick-start: apply full PWM (255) for 500ms
    analogWrite(FAN_PWM_PIN, 255);
    delay(500);
  }
  
  currentFanSpeed = speedLevel;
  int pwmValue;
  switch (speedLevel) {
    case 0: pwmValue = 0; break;
    case 1: pwmValue = 100; break; // Increased from 51 for better torque
    case 2: pwmValue = 128; break; // ~50%
    case 3: pwmValue = 153; break; // ~60%
    case 4: pwmValue = 204; break; // ~80%
    case 5: pwmValue = 255; break; // 100%
    default: pwmValue = 0; break;
  }
  analogWrite(FAN_PWM_PIN, pwmValue);
  Serial.print("Fan Speed Set: ");
  Serial.println(speedLevel);
}

void receiveEvent(int howMany) {
  while (Wire.available()) {
    byte cmd = Wire.read();
    
    switch (cmd) {
      case CMD_LIGHT_ON:
        if (!fireAlarmActive) {
          digitalWrite(LIGHT_PIN, HIGH);
          Serial.println("Light: ON");
        }
        break;
      case CMD_LIGHT_OFF:
        digitalWrite(LIGHT_PIN, LOW);
        Serial.println("Light: OFF");
        break;
      case CMD_LOCK:
        if (!fireAlarmActive) {
          doorLockServo.write(LOCK_ANGLE);
          digitalWrite(DOOR_LED_PIN, LOW);
          Serial.println("Door: LOCKED");
        }
        break;
      case CMD_UNLOCK:
        doorLockServo.write(UNLOCK_ANGLE);
        digitalWrite(DOOR_LED_PIN, HIGH);
        Serial.println("Door: UNLOCKED");
        break;
      case CMD_WARNING_BLINK:
        if (!fireAlarmActive) {
          startWarningBlink();
          Serial.println("Warning Blink: STARTED");
        }
        break;
      case CMD_WRONG_PIN_BLINK:
        if (!fireAlarmActive) {
          startWrongPinBlink();
          Serial.println("Wrong PIN Blink: STARTED");
        }
        break;
      case CMD_FAN_ON:
        if (!fireAlarmActive) {
          setFanSpeed(3);
          Serial.println("Fan: ON (Speed 3)");
        }
        break;
      case CMD_FAN_OFF:
        setFanSpeed(0);
        Serial.println("Fan: OFF");
        break;
      case CMD_FAN_SPEED:
        if (Wire.available() && !fireAlarmActive) {
          byte speedLevel = Wire.read();
          setFanSpeed(speedLevel);
          Serial.print("Fan Speed: ");
          Serial.println(speedLevel);
        }
        break;
      case CMD_AUTO_OFF_BLINK:
        if (!fireAlarmActive) {
          startAutoModeBlink();
          Serial.println("Auto Mode Blink: STARTED");
        }
        break;
      case CMD_FIRE_ALARM:
        fireAlarm();
        break;
      case CMD_STOP_BLINK:
        stopAllBlinking();
        Serial.println("All Blinking: STOPPED");
        break;
    }
  }
}