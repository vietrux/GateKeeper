/*
 * ═══════════════════════════════════════════════════════════════════════════
 * GateKeeper Firmware - ESP32 Smart Gate Controller
 * ═══════════════════════════════════════════════════════════════════════════
 * Hardware: ESP32, LM393 Sensor, SG90 Servo, Logic Level Shifter
 * See hardware wiring guide at end of file
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ═══════════════════════════════════════════════════════════════════════════
// CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════

namespace Config {
  // WiFi Credentials
  constexpr char WIFI_SSID[] = "TP-Link_4736";
  constexpr char WIFI_PASSWORD[] = "QuakQuak@1238";
  constexpr unsigned long WIFI_TIMEOUT_MS = 20000;
  
  // Webhook Configuration
  constexpr char WEBHOOK_URL[] = "http://192.168.10.213:8000/lpr";
  constexpr unsigned long HTTP_TIMEOUT_MS = 60000;
  
  // Hardware Pins
  constexpr int LM393_SENSOR_PIN = 4;
  constexpr int SERVO_CONTROL_PIN = 5;
  
  // Servo Settings
  constexpr int SERVO_MIN_PULSE_US = 500;
  constexpr int SERVO_MAX_PULSE_US = 2400;
  constexpr int SERVO_CLOSED_ANGLE = 0;
  constexpr int SERVO_OPEN_ANGLE = 90;

  // OLED Display Settings
  constexpr int OLED_WIDTH = 128;
  constexpr int OLED_HEIGHT = 64;
  constexpr int OLED_RESET_PIN = -1;
  constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;
  
  // Timing
  constexpr unsigned long DEBOUNCE_DELAY_MS = 50;
  constexpr unsigned long LOOP_DELAY_MS = 10;
  constexpr int SERIAL_BAUD_RATE = 115200;
}

// ═══════════════════════════════════════════════════════════════════════════
// WIFI MANAGER
// ═══════════════════════════════════════════════════════════════════════════

class WiFiManager {
public:
  static void connect() {
    if (isConnected()) {
      return;
    }

    Serial.println("[WiFi] Connecting...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);

    const unsigned long startTime = millis();
    
    while (!isConnected() && !isTimeout(startTime)) {
      delay(500);
      Serial.print('.');
    }
    
    Serial.println();
    
    if (isConnected()) {
      Serial.print("[WiFi] Connected. IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("[WiFi] Connection failed. Will retry later.");
    }
  }

  static bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
  }

private:
  static bool isTimeout(unsigned long startTime) {
    return (millis() - startTime) >= Config::WIFI_TIMEOUT_MS;
  }
};

// ═══════════════════════════════════════════════════════════════════════════
// HTTP CLIENT
// ═══════════════════════════════════════════════════════════════════════════

class WebhookClient {
public:
  static bool shouldOpenGate(String& plateOut) {
    plateOut = "";
    if (!WiFiManager::isConnected()) {
      Serial.println("[HTTP] Skipping GET - WiFi not connected");
      return false;
    }

    HTTPClient http;
    Serial.printf("[HTTP] GET %s\n", Config::WEBHOOK_URL);

    if (!http.begin(Config::WEBHOOK_URL)) {
      Serial.println("[HTTP] Unable to begin connection");
      return false;
    }

    http.setTimeout(Config::HTTP_TIMEOUT_MS);
    const int responseCode = http.GET();

    if (responseCode <= 0) {
      Serial.printf("[HTTP] Request failed: %s\n",
                    http.errorToString(responseCode).c_str());
      http.end();
      return false;
    }

    Serial.printf("[HTTP] Response code: %d\n", responseCode);

    bool gateStatus = false;

    if (responseCode == HTTP_CODE_OK) {
      const String payload = http.getString();
      Serial.printf("[HTTP] Payload: %s\n", payload.c_str());

      bool parsedStatus = false;
      if (parseStatusField(payload, parsedStatus)) {
        gateStatus = parsedStatus;
      } else {
        Serial.println("[HTTP] Unable to parse status field");
      }

      String parsedPlate;
      if (parsePlateField(payload, parsedPlate)) {
        plateOut = parsedPlate;
      }
    }

    http.end();
    return gateStatus;
  }

private:
  static bool parseStatusField(const String& payload, bool& status) {
    const int statusIndex = payload.indexOf("\"status\"");
    if (statusIndex < 0) {
      return false;
    }

    int colonIndex = payload.indexOf(':', statusIndex);
    if (colonIndex < 0) {
      return false;
    }

    colonIndex = skipWhitespace(payload, colonIndex + 1);
    if (colonIndex < 0) {
      return false;
    }

    if (payload.startsWith("true", colonIndex)) {
      status = true;
      return true;
    }

    if (payload.startsWith("false", colonIndex)) {
      status = false;
      return true;
    }

    return false;
  }

  static int skipWhitespace(const String& text, int index) {
    while (index < text.length()) {
      const char ch = text.charAt(index);
      if (ch != ' ' && ch != '\n' && ch != '\r' && ch != '\t') {
        return index;
      }
      ++index;
    }
    return -1;
  }

  static bool parsePlateField(const String& payload, String& plate) {
    const int plateIndex = payload.indexOf("\"plate\"");
    if (plateIndex < 0) {
      return false;
    }

    int colonIndex = payload.indexOf(':', plateIndex);
    if (colonIndex < 0) {
      return false;
    }

    colonIndex = skipWhitespace(payload, colonIndex + 1);
    if (colonIndex < 0 || payload.charAt(colonIndex) != '"') {
      return false;
    }

    const int endQuoteIndex = payload.indexOf('"', colonIndex + 1);
    if (endQuoteIndex < 0) {
      return false;
    }

    plate = payload.substring(colonIndex + 1, endQuoteIndex);
    return true;
  }
};

// ═══════════════════════════════════════════════════════════════════════════
// OLED DISPLAY MANAGER
// ═══════════════════════════════════════════════════════════════════════════

class DisplayManager {
public:
  void initialize() {
    if (initialized_) {
      return;
    }

    if (!display_.begin(SSD1306_SWITCHCAPVCC, Config::OLED_I2C_ADDRESS)) {
      Serial.println("[OLED] Initialization failed");
      return;
    }

    initialized_ = true;
    display_.clearDisplay();
    display_.display();
    display_.setTextColor(SSD1306_WHITE);
    display_.setTextSize(2);
    showWelcome();
  }

  void showWelcome() {
    showLines("Welcome");
  }

  void showCarChecking() {
    showLines("CAR", "", "checking");
  }

  void showAccept(const String& plate = String()) {
    showStatusWithPlate("ACCEPT", plate);
  }

  void showDeny(const String& plate = String()) {
    showStatusWithPlate("DENY", plate);
  }

private:
  Adafruit_SSD1306 display_{Config::OLED_WIDTH, Config::OLED_HEIGHT, &Wire, Config::OLED_RESET_PIN};
  bool initialized_ = false;

  void showLines(const char* line1, const char* line2 = nullptr, const char* line3 = nullptr) {
    if (!initialized_) {
      return;
    }

    display_.clearDisplay();
    display_.setCursor(0, 0);

    if (line1 != nullptr) {
      display_.println(line1);
    }

    if (line2 != nullptr) {
      display_.println(line2);
    }

    if (line3 != nullptr) {
      display_.println(line3);
    }

    display_.display();
  }

  void showStatusWithPlate(const char* status, const String& plate) {
    if (!initialized_) {
      return;
    }

    display_.clearDisplay();
    display_.setCursor(0, 0);
    display_.println(status);
    display_.println();

    if (plate.length() > 0) {
      display_.println(plate);
    }

    display_.display();
  }
};

// ═══════════════════════════════════════════════════════════════════════════
// SERVO CONTROLLER
// ═══════════════════════════════════════════════════════════════════════════

class ServoController {
public:
  void initialize() {
    servo_.attach(Config::SERVO_CONTROL_PIN, 
                  Config::SERVO_MIN_PULSE_US, 
                  Config::SERVO_MAX_PULSE_US);
    close();
    delay(500);
  }

  void open() {
    moveTo(Config::SERVO_OPEN_ANGLE);
  }

  void close() {
    moveTo(Config::SERVO_CLOSED_ANGLE);
  }

private:
  Servo servo_;

  void moveTo(int angle) {
    servo_.write(angle);
    Serial.printf("[Servo] Moving to %d degrees\n", angle);
  }
};

// ═══════════════════════════════════════════════════════════════════════════
// SENSOR READER WITH DEBOUNCING
// ═══════════════════════════════════════════════════════════════════════════

class DebouncedSensor {
public:
  void initialize(int pin) {
    pin_ = pin;
    pinMode(pin_, INPUT);
    
    // Initialize debounced state
    int initialValue = digitalRead(pin_);
    rawValue_ = initialValue;
    stableValue_ = initialValue;
    lastBounceTime_ = millis();
  }

  bool hasChanged() {
    updateRawValue();
    
    if (isDebounceDelayElapsed() && hasStableValueChanged()) {
      stableValue_ = rawValue_;
      Serial.printf("[Sensor] LM393 state changed: %d\n", stableValue_);
      return true;
    }
    
    return false;
  }

  int getStableValue() const {
    return stableValue_;
  }

private:
  int pin_;
  int rawValue_ = -1;
  int stableValue_ = -1;
  unsigned long lastBounceTime_ = 0;

  void updateRawValue() {
    const int currentRaw = digitalRead(pin_);
    
    if (currentRaw != rawValue_) {
      rawValue_ = currentRaw;
      lastBounceTime_ = millis();
    }
  }

  bool isDebounceDelayElapsed() const {
    return (millis() - lastBounceTime_) >= Config::DEBOUNCE_DELAY_MS;
  }

  bool hasStableValueChanged() const {
    return rawValue_ != stableValue_;
  }
};

// ═══════════════════════════════════════════════════════════════════════════
// MAIN APPLICATION
// ═══════════════════════════════════════════════════════════════════════════

class GateKeeperApp {
public:
  void setup() {
    initializeSerial();
    initializeHardware();
    WiFiManager::connect();
  }

  void loop() {
    ensureWiFiConnected();
    processSensorInput();
    delay(Config::LOOP_DELAY_MS);
  }

private:
  DebouncedSensor sensor_;
  ServoController servo_;
  DisplayManager display_;

  void initializeSerial() {
    Serial.begin(Config::SERIAL_BAUD_RATE);
    delay(100);
  }

  void initializeHardware() {
    display_.initialize();
    sensor_.initialize(Config::LM393_SENSOR_PIN);
    servo_.initialize();
  }

  void ensureWiFiConnected() {
    if (!WiFiManager::isConnected()) {
      WiFiManager::connect();
    }
  }

  void processSensorInput() {
    if (sensor_.hasChanged()) {
      const int sensorValue = sensor_.getStableValue();
      if (sensorValue == 0) {
        display_.showCarChecking();
        String plate;
        const bool shouldOpen = WebhookClient::shouldOpenGate(plate);
        if (shouldOpen) {
          display_.showAccept(plate);
        } else {
          display_.showDeny(plate);
        }
        updateServoPosition(shouldOpen ? 1 : 0);
      } else {
        display_.showWelcome();
        updateServoPosition(0);
      }
    }
  }

  void updateServoPosition(int sensorValue) {
    if (sensorValue == 1) {
      servo_.open();
    } else {
      servo_.close();
    }
  }
};

// ═══════════════════════════════════════════════════════════════════════════
// ARDUINO ENTRY POINTS
// ═══════════════════════════════════════════════════════════════════════════

GateKeeperApp app;

void setup() {
  app.setup();
}

void loop() {
  app.loop();
}

/*
 * ═══════════════════════════════════════════════════════════════════════════
 * HARDWARE WIRING GUIDE
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * COMPONENTS:
 *   • ESP32 (upesy_wroom)
 *   • LM393 Speed/Obstacle Sensor
 *   • Bidirectional Logic Level Shifter (3.3V ↔ 5V)
 *   • SG90 Servo Motor
 *   • External 5V Power Supply (recommended)
 * 
 * ───────────────────────────────────────────────────────────────────────────
 * LM393 SENSOR:
 * ───────────────────────────────────────────────────────────────────────────
 *   LM393 VCC  →  3.3V or 5V
 *   LM393 GND  →  GND
 *   LM393 DO   →  ESP32 GPIO 4
 * 
 * ───────────────────────────────────────────────────────────────────────────
 * LOGIC LEVEL SHIFTER:
 * ───────────────────────────────────────────────────────────────────────────
 *   LV Side (3.3V):
 *     LV      →  ESP32 3.3V
 *     GND     →  ESP32 GND
 *     LV1     →  ESP32 GPIO 5
 * 
 *   HV Side (5V):
 *     HV      →  5V Power Supply
 *     GND     →  Common GND
 *     HV1     →  SG90 Signal Wire
 * 
 * ───────────────────────────────────────────────────────────────────────────
 * SG90 SERVO:
 * ───────────────────────────────────────────────────────────────────────────
 *   Brown/Black   →  GND (common ground)
 *   Red           →  5V External Power Supply
 *   Orange/Yellow →  Logic Level Shifter HV1
 * 
 * ───────────────────────────────────────────────────────────────────────────
 * ⚠️  CRITICAL NOTES:
 * ───────────────────────────────────────────────────────────────────────────
 *   • ALL grounds must be connected together
 *   • Use EXTERNAL 5V power for servo (up to 500mA draw)
 *   • DO NOT power servo from ESP32 5V pin
 *   • Logic level shifter prevents voltage mismatch
 *   • Servo requires 5V logic for reliable operation
 * 
 * ═══════════════════════════════════════════════════════════════════════════
 */