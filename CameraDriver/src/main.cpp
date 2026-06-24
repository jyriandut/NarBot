#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include "esp_camera.h"

// DFRobot TB6612FNG 2x1.2A motor driver on XIAO Arduino pins.
const int LEFT_DIR_PIN = D7;    // DIR1
const int LEFT_PWM_PIN = D8;    // PWM1
const int RIGHT_DIR_PIN = D10;  // DIR2
const int RIGHT_PWM_PIN = D9;   // PWM2
const int LEFT_PWM_CHANNEL = 2;
const int RIGHT_PWM_CHANNEL = 3;
const int MOTOR_PWM_FREQ = 20000;
const int MOTOR_PWM_RESOLUTION = 8;
const int MOTOR_SPEED = 220;  // 0-255

const int STATUS_LED = LED_BUILTIN;
const int I2C_SDA_PIN = 5;  // XIAO D4
const int I2C_SCL_PIN = 6;  // XIAO D5
const uint8_t COLOR_SENSOR_ADDR = 0x29;
const unsigned long COLOR_READ_INTERVAL_MS = 250;

const uint8_t TCS_COMMAND_BIT = 0x80;
const uint8_t TCS_ENABLE = 0x00;
const uint8_t TCS_ATIME = 0x01;
const uint8_t TCS_CONTROL = 0x0F;
const uint8_t TCS_ID = 0x12;
const uint8_t TCS_STATUS = 0x13;
const uint8_t TCS_CDATAL = 0x14;
const uint8_t TCS_RDATAL = 0x16;
const uint8_t TCS_GDATAL = 0x18;
const uint8_t TCS_BDATAL = 0x1A;
const uint8_t TCS_ENABLE_PON = 0x01;
const uint8_t TCS_ENABLE_AEN = 0x02;
const uint8_t TCS_STATUS_AVALID = 0x01;
const uint8_t TCS_INTEGRATIONTIME_154MS = 0xC0;
const uint8_t TCS_GAIN_4X = 0x01;

const char *SETUP_AP_SSID = "CameraDriver-Setup";
const char *SETUP_AP_PASSWORD = "smars1234";
const char *LOCAL_DNS_NAME = "narbot.esp";
const byte DNS_PORT = 53;
const unsigned long STREAM_FRAME_INTERVAL_MS = 120;

// Seeed Studio XIAO ESP32S3 Sense camera pins.
const int CAM_PIN_PWDN = -1;
const int CAM_PIN_RESET = -1;
const int CAM_PIN_XCLK = 10;
const int CAM_PIN_SIOD = 40;
const int CAM_PIN_SIOC = 39;
const int CAM_PIN_Y9 = 48;
const int CAM_PIN_Y8 = 11;
const int CAM_PIN_Y7 = 12;
const int CAM_PIN_Y6 = 14;
const int CAM_PIN_Y5 = 16;
const int CAM_PIN_Y4 = 18;
const int CAM_PIN_Y3 = 17;
const int CAM_PIN_Y2 = 15;
const int CAM_PIN_VSYNC = 38;
const int CAM_PIN_HREF = 47;
const int CAM_PIN_PCLK = 13;

struct LedPatternStep {
  bool on;
  unsigned long durationMs;
};

struct LedPattern {
  const LedPatternStep *steps;
  size_t stepCount;
};

struct ColorReading {
  bool available;
  uint16_t clear;
  uint16_t red;
  uint16_t green;
  uint16_t blue;
  uint8_t displayRed;
  uint8_t displayGreen;
  uint8_t displayBlue;
};

enum SerialConsoleMode {
  SERIAL_CONSOLE_MAIN,
  SERIAL_CONSOLE_DRIVE,
  SERIAL_CONSOLE_WIFI,
};

enum MotorDirection {
  MOTOR_STOPPED,
  MOTOR_FORWARD,
  MOTOR_BACKWARD,
};

const LedPatternStep STOP_LED_STEPS[] = {
    {true, 50},
    {false, 1950},
};

const LedPatternStep FORWARD_LED_STEPS[] = {
    {true, 40},
    {false, 210},
};

const LedPatternStep BACKWARD_LED_STEPS[] = {
    {true, 70},
    {false, 120},
    {true, 70},
    {false, 740},
};

const LedPatternStep LEFT_LED_STEPS[] = {
    {true, 250},
    {false, 750},
};

const LedPatternStep RIGHT_LED_STEPS[] = {
    {true, 60},
    {false, 120},
    {true, 60},
    {false, 120},
    {true, 60},
    {false, 580},
};

const LedPattern STOP_LED_PATTERN = {STOP_LED_STEPS, 2};
const LedPattern FORWARD_LED_PATTERN = {FORWARD_LED_STEPS, 2};
const LedPattern BACKWARD_LED_PATTERN = {BACKWARD_LED_STEPS, 4};
const LedPattern LEFT_LED_PATTERN = {LEFT_LED_STEPS, 2};
const LedPattern RIGHT_LED_PATTERN = {RIGHT_LED_STEPS, 6};

const LedPattern *currentLedPattern = &STOP_LED_PATTERN;
size_t currentLedStep = 0;
unsigned long ledStepStartedAt = 0;
bool cameraReady = false;
bool webServerStarted = false;
bool restartRequested = false;
unsigned long restartAt = 0;
bool headlightsEnabled = false;
MotorDirection leftMotorState = MOTOR_STOPPED;
MotorDirection rightMotorState = MOTOR_STOPPED;
int leftMotorPwm = 0;
int rightMotorPwm = 0;
bool colorSensorReady = false;
uint8_t colorSensorId = 0;
ColorReading latestColor = {};
unsigned long lastColorReadAt = 0;
String serialLine;
SerialConsoleMode serialMode = SERIAL_CONSOLE_MAIN;

WebServer server(80);
DNSServer dnsServer;
WiFiServer streamServer(81);
WiFiClient streamClient;
unsigned long lastStreamFrameAt = 0;

void stopMotors();
void driveForward();
void driveBackward();
void turnLeft();
void turnRight();
void testLeftForward();
void testLeftBackward();
void testRightForward();
void testRightBackward();
void setStatusLedPattern(const LedPattern &pattern);
void updateStatusLed();
void writeStatusLed(bool on);
void setHeadlights(bool enabled);
void handleSerialCommand(char cmd);
void printCommandHelp();
void printMainHelp();
void printDriveHelp();
void initCamera();
void initColorSensor();
void updateColorSensor();
bool readColorSensor(ColorReading &reading);
void normalizeColorReading(ColorReading &reading);
void handleColorState();
bool writeColorRegister(uint8_t reg, uint8_t value);
bool readColorRegister(uint8_t reg, uint8_t &value);
bool readColorRegister16(uint8_t reg, uint16_t &value);
void captureAndSendImage();
camera_fb_t *captureFreshFrame();
void handleSerialInput();
void handleSerialLine(const String &line);
void handleMainSerialLine(const String &command);
void handleDriveSerialLine(const String &command);
void handleWifiSerialLine(const String &command);
void printWifiHelp();
void scanWifiNetworks();
void startSetupAccessPoint();
void startLocalDns();
void startWebServer();
void startStreamServer();
void updateStreamServer();
void sendStreamHeaders(WiFiClient &client);
void sendStreamFrame(WiFiClient &client);
void handleRoot();
void handleWifiSetupPage();
void handleWifiSave();
void handleWebCapture();
void handleMotorState();
void sendPlainText(const String &message);
void printNetworkStatus();
void requestRestart();
void setMotorState(MotorDirection leftDirection, MotorDirection rightDirection);
void writeMotor(int dirPin, int pwmChannel, MotorDirection direction, int speed);
const char *motorDirectionName(MotorDirection direction);

void setup() {
  pinMode(LEFT_PWM_PIN, OUTPUT);
  pinMode(RIGHT_PWM_PIN, OUTPUT);
  pinMode(LEFT_DIR_PIN, OUTPUT);
  pinMode(RIGHT_DIR_PIN, OUTPUT);
  digitalWrite(LEFT_PWM_PIN, LOW);
  digitalWrite(RIGHT_PWM_PIN, LOW);
  digitalWrite(LEFT_DIR_PIN, LOW);
  digitalWrite(RIGHT_DIR_PIN, LOW);

  Serial.begin(115200);

  ledcSetup(LEFT_PWM_CHANNEL, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
  ledcSetup(RIGHT_PWM_CHANNEL, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
  ledcWrite(LEFT_PWM_CHANNEL, 0);
  ledcWrite(RIGHT_PWM_CHANNEL, 0);
  ledcAttachPin(LEFT_PWM_PIN, LEFT_PWM_CHANNEL);
  ledcAttachPin(RIGHT_PWM_PIN, RIGHT_PWM_CHANNEL);
  pinMode(STATUS_LED, OUTPUT);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  stopMotors();

  Serial.println();
  Serial.println("XIAO ESP32S3 Sense + TB6612FNG motor test");
  Serial.println("Status LED blinking on LED_BUILTIN");
  Serial.println("LED patterns:");
  Serial.println("  stop = slow heartbeat");
  Serial.println("  forward = quick steady tick");
  Serial.println("  backward = double flash");
  Serial.println("  left = long blink");
  Serial.println("  right = triple flash");
  printMainHelp();

  startSetupAccessPoint();
  startLocalDns();
  startWebServer();
  startStreamServer();
  initColorSensor();
  initCamera();
}

void loop() {
  updateStatusLed();
  updateColorSensor();
  handleSerialInput();
  dnsServer.processNextRequest();
  server.handleClient();
  updateStreamServer();

  if (restartRequested && millis() >= restartAt) {
    ESP.restart();
  }
}

void handleSerialInput() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (serialLine.length() > 0) {
        handleSerialLine(serialLine);
        serialLine = "";
      }
      continue;
    }

    if (serialLine.length() < 96) {
      serialLine += c;
    }
  }
}

void handleSerialLine(const String &line) {
  String command = line;
  command.trim();

  if (command.length() == 0) {
    return;
  }

  if (serialMode == SERIAL_CONSOLE_MAIN) {
    handleMainSerialLine(command);
  } else if (serialMode == SERIAL_CONSOLE_DRIVE) {
    handleDriveSerialLine(command);
  } else if (serialMode == SERIAL_CONSOLE_WIFI) {
    handleWifiSerialLine(command);
  }
}

void handleMainSerialLine(const String &command) {
  if (command == "/drive") {
    serialMode = SERIAL_CONSOLE_DRIVE;
    printDriveHelp();
    return;
  }

  if (command == "/wifi") {
    serialMode = SERIAL_CONSOLE_WIFI;
    printWifiHelp();
    return;
  }

  if (command == "?" || command == "help" || command == "/help") {
    printMainHelp();
    return;
  }

  Serial.print("Unknown main command: ");
  Serial.println(command);
  printMainHelp();
}

void handleDriveSerialLine(const String &command) {
  if (command == "q" || command == "Q") {
    stopMotors();
    serialMode = SERIAL_CONSOLE_MAIN;
    Serial.println("Exited drive mode");
    printMainHelp();
    return;
  }

  if (command == "?" || command == "h" || command == "H" || command == "help") {
    printDriveHelp();
    return;
  }

  if (command.length() == 1) {
    handleSerialCommand(command[0]);
    return;
  }

  Serial.print("Unknown drive command: ");
  Serial.println(command);
  printDriveHelp();
}

void handleWifiSerialLine(const String &command) {
  if (command == "q" || command == "Q") {
    serialMode = SERIAL_CONSOLE_MAIN;
    Serial.println("Exited Wi-Fi mode");
    printMainHelp();
    return;
  }

  if (command == "scan") {
    scanWifiNetworks();
    return;
  }

  if (command == "status") {
    printNetworkStatus();
    return;
  }

  if (command == "ap") {
    startSetupAccessPoint();
    return;
  }

  if (command == "help" || command == "?" || command == "h" || command == "H") {
    printWifiHelp();
    return;
  }

  Serial.print("Unknown Wi-Fi command: ");
  Serial.println(command);
  printWifiHelp();
}

void handleSerialCommand(char cmd) {
  switch (cmd) {
      case 'w':
      case 'W':
        Serial.println("Forward");
        driveForward();
        break;

      case 's':
      case 'S':
        Serial.println("Backward");
        driveBackward();
        break;

      case 'a':
      case 'A':
        Serial.println("Left");
        turnLeft();
        break;

      case 'd':
      case 'D':
        Serial.println("Right");
        turnRight();
        break;

    case 'x':
    case 'X':
    case ' ':
      Serial.println("Stop");
      stopMotors();
      break;

    case '1':
      Serial.println("Left motor forward");
      testLeftForward();
      break;

    case '2':
      Serial.println("Left motor backward");
      testLeftBackward();
      break;

    case '3':
      Serial.println("Right motor forward");
      testRightForward();
      break;

    case '4':
      Serial.println("Right motor backward");
      testRightBackward();
      break;

    case 'u':
    case 'U':
      Serial.println("Capture");
      stopMotors();
      captureAndSendImage();
      break;

    case 'o':
    case 'O':
      setHeadlights(true);
      Serial.println("Status LED on");
      break;

    case 'p':
    case 'P':
      setHeadlights(false);
      Serial.println("Status LED pattern");
      break;

    case '?':
    case 'h':
    case 'H':
      printCommandHelp();
      break;

    case '\n':
    case '\r':
    case '\t':
      break;

    default:
      Serial.print("Unknown command: ");
      Serial.println(cmd);
      printCommandHelp();
      break;
  }
}

void printCommandHelp() {
  printDriveHelp();
}

void printMainHelp() {
  Serial.println("Serial modes:");
  Serial.println("  /drive = enter drive command mode");
  Serial.println("  /wifi = enter Wi-Fi command mode");
  Serial.println("  /help = show this help");
  Serial.println();
}

void printDriveHelp() {
  Serial.println("Drive mode commands:");
  Serial.println("  w = forward");
  Serial.println("  s = backward");
  Serial.println("  a = left");
  Serial.println("  d = right");
  Serial.println("  x or space = stop");
  Serial.println("  1 = left motor forward");
  Serial.println("  2 = left motor backward");
  Serial.println("  3 = right motor forward");
  Serial.println("  4 = right motor backward");
  Serial.println("  u = capture JPEG over serial");
  Serial.println("  o = status LED on");
  Serial.println("  p = status LED pattern");
  Serial.println("  ? = help");
  Serial.println("  q = quit drive mode");
  Serial.println();
}

void printWifiHelp() {
  Serial.println("Wi-Fi mode commands:");
  Serial.println("  scan");
  Serial.println("  status");
  Serial.println("  ap = restart setup access point");
  Serial.println("  help");
  Serial.println("  q = quit Wi-Fi mode");
  Serial.println();
}

void initCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_1;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = CAM_PIN_Y2;
  config.pin_d1 = CAM_PIN_Y3;
  config.pin_d2 = CAM_PIN_Y4;
  config.pin_d3 = CAM_PIN_Y5;
  config.pin_d4 = CAM_PIN_Y6;
  config.pin_d5 = CAM_PIN_Y7;
  config.pin_d6 = CAM_PIN_Y8;
  config.pin_d7 = CAM_PIN_Y9;
  config.pin_xclk = CAM_PIN_XCLK;
  config.pin_pclk = CAM_PIN_PCLK;
  config.pin_vsync = CAM_PIN_VSYNC;
  config.pin_href = CAM_PIN_HREF;
  config.pin_sccb_sda = CAM_PIN_SIOD;
  config.pin_sccb_scl = CAM_PIN_SIOC;
  config.pin_pwdn = CAM_PIN_PWDN;
  config.pin_reset = CAM_PIN_RESET;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.jpeg_quality = 12;
  config.grab_mode = CAMERA_GRAB_LATEST;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_QVGA;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    Serial.println("Camera using PSRAM frame buffer");
  } else {
    config.frame_size = FRAMESIZE_VGA;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
    Serial.println("No PSRAM found; camera using VGA DRAM frame buffer");
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    cameraReady = false;
    return;
  }

  cameraReady = true;
  Serial.println("Camera ready");
}

void initColorSensor() {
  if (!readColorRegister(TCS_ID, colorSensorId)) {
    colorSensorReady = false;
    Serial.println("Color sensor not found at I2C address 0x29");
    return;
  }

  colorSensorReady = writeColorRegister(TCS_ENABLE, TCS_ENABLE_PON);
  delay(3);
  colorSensorReady = colorSensorReady && writeColorRegister(TCS_ATIME, TCS_INTEGRATIONTIME_154MS);
  colorSensorReady = colorSensorReady && writeColorRegister(TCS_CONTROL, TCS_GAIN_4X);
  colorSensorReady = colorSensorReady && writeColorRegister(TCS_ENABLE, TCS_ENABLE_PON | TCS_ENABLE_AEN);

  if (!colorSensorReady) {
    Serial.println("Color sensor init failed");
    return;
  }

  Serial.print("Color sensor ready, ID 0x");
  Serial.println(colorSensorId, HEX);
}

void updateColorSensor() {
  if (!colorSensorReady || millis() - lastColorReadAt < COLOR_READ_INTERVAL_MS) {
    return;
  }

  lastColorReadAt = millis();
  readColorSensor(latestColor);
}

bool readColorSensor(ColorReading &reading) {
  uint8_t status = 0;
  if (!readColorRegister(TCS_STATUS, status) || (status & TCS_STATUS_AVALID) == 0) {
    reading.available = false;
    return false;
  }

  bool ok = readColorRegister16(TCS_CDATAL, reading.clear);
  ok = ok && readColorRegister16(TCS_RDATAL, reading.red);
  ok = ok && readColorRegister16(TCS_GDATAL, reading.green);
  ok = ok && readColorRegister16(TCS_BDATAL, reading.blue);

  reading.available = ok;
  if (ok) {
    normalizeColorReading(reading);
  }

  return ok;
}

void normalizeColorReading(ColorReading &reading) {
  if (reading.clear == 0) {
    reading.displayRed = 0;
    reading.displayGreen = 0;
    reading.displayBlue = 0;
    return;
  }

  uint32_t red = (uint32_t)reading.red * 256 / reading.clear;
  uint32_t green = (uint32_t)reading.green * 256 / reading.clear;
  uint32_t blue = (uint32_t)reading.blue * 256 / reading.clear;

  reading.displayRed = min<uint32_t>(red, 255);
  reading.displayGreen = min<uint32_t>(green, 255);
  reading.displayBlue = min<uint32_t>(blue, 255);
}

bool writeColorRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(COLOR_SENSOR_ADDR);
  Wire.write(TCS_COMMAND_BIT | reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readColorRegister(uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(COLOR_SENSOR_ADDR);
  Wire.write(TCS_COMMAND_BIT | reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom(COLOR_SENSOR_ADDR, (uint8_t)1) != 1) {
    return false;
  }

  value = Wire.read();
  return true;
}

bool readColorRegister16(uint8_t reg, uint16_t &value) {
  uint8_t low = 0;
  uint8_t high = 0;

  if (!readColorRegister(reg, low) || !readColorRegister(reg + 1, high)) {
    return false;
  }

  value = ((uint16_t)high << 8) | low;
  return true;
}

void captureAndSendImage() {
  if (!cameraReady) {
    Serial.println("ERR camera not ready");
    return;
  }

  camera_fb_t *fb = captureFreshFrame();
  if (fb == nullptr) {
    Serial.println("ERR camera capture failed");
    return;
  }

  Serial.printf("IMG %u\n", fb->len);
  Serial.write(fb->buf, fb->len);
  Serial.println();
  Serial.println("END");

  esp_camera_fb_return(fb);
}

camera_fb_t *captureFreshFrame() {
  camera_fb_t *staleFrame = esp_camera_fb_get();
  if (staleFrame != nullptr) {
    esp_camera_fb_return(staleFrame);
    delay(100);
  }

  camera_fb_t *fb = nullptr;
  for (int attempt = 1; attempt <= 3 && fb == nullptr; attempt++) {
    fb = esp_camera_fb_get();
    if (fb == nullptr) {
      Serial.printf("Capture attempt %d failed\n", attempt);
      delay(200);
    }
  }

  return fb;
}

void scanWifiNetworks() {
  Serial.println("Scanning Wi-Fi networks...");
  int networkCount = WiFi.scanNetworks();

  if (networkCount <= 0) {
    Serial.println("No Wi-Fi networks found");
    return;
  }

  for (int i = 0; i < networkCount; i++) {
    Serial.printf("  %d: %s (%d dBm)%s\n",
                  i + 1,
                  WiFi.SSID(i).c_str(),
                  WiFi.RSSI(i),
                  WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? " open" : "");
  }
}

void startSetupAccessPoint() {
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(SETUP_AP_SSID, SETUP_AP_PASSWORD);

  if (!ok) {
    Serial.println("ERR failed to start setup AP");
    return;
  }

  Serial.print("Setup AP started: ");
  Serial.println(SETUP_AP_SSID);
  Serial.print("Setup AP password: ");
  Serial.println(SETUP_AP_PASSWORD);
  Serial.print("Setup page: http://");
  Serial.println(WiFi.softAPIP());
}

void startLocalDns() {
  bool ok = dnsServer.start(DNS_PORT, LOCAL_DNS_NAME, WiFi.softAPIP());

  if (!ok) {
    Serial.println("ERR failed to start local DNS");
    return;
  }

  Serial.print("Local DNS started: http://");
  Serial.println(LOCAL_DNS_NAME);
}

void startWebServer() {
  if (webServerStarted) {
    return;
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/wifi", HTTP_GET, handleWifiSetupPage);
  server.on("/wifi-save", HTTP_POST, handleWifiSave);
  server.on("/capture", HTTP_GET, handleWebCapture);
  server.on("/motor-state", HTTP_GET, handleMotorState);
  server.on("/color-state", HTTP_GET, handleColorState);

  server.on("/forward", HTTP_GET, []() {
    driveForward();
    sendPlainText("Forward");
  });
  server.on("/backward", HTTP_GET, []() {
    driveBackward();
    sendPlainText("Backward");
  });
  server.on("/left", HTTP_GET, []() {
    turnLeft();
    sendPlainText("Left");
  });
  server.on("/right", HTTP_GET, []() {
    turnRight();
    sendPlainText("Right");
  });
  server.on("/left-forward", HTTP_GET, []() {
    testLeftForward();
    sendPlainText("Left motor forward");
  });
  server.on("/left-backward", HTTP_GET, []() {
    testLeftBackward();
    sendPlainText("Left motor backward");
  });
  server.on("/right-forward", HTTP_GET, []() {
    testRightForward();
    sendPlainText("Right motor forward");
  });
  server.on("/right-backward", HTTP_GET, []() {
    testRightBackward();
    sendPlainText("Right motor backward");
  });
  server.on("/stop", HTTP_GET, []() {
    stopMotors();
    sendPlainText("Stop");
  });
  server.on("/headlights-on", HTTP_GET, []() {
    setHeadlights(true);
    sendPlainText("Status LED on");
  });
  server.on("/headlights-off", HTTP_GET, []() {
    setHeadlights(false);
    sendPlainText("Status LED pattern");
  });

  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });

  server.begin();
  webServerStarted = true;

  Serial.println("Web server started");
  printNetworkStatus();
}

void handleRoot() {
  String localIp = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "not connected";
  String apIp = WiFi.softAPIP().toString();

  String html = R"HTML(
<!doctype html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>CameraDriver</title>
  <style>
    body { font-family: system-ui, sans-serif; margin: 0; padding: 24px; background: #f6f7f9; color: #111; }
    main { max-width: 760px; margin: 0 auto; }
    h1 { font-size: 28px; margin: 0 0 8px; }
    section { margin-top: 24px; }
    .grid { display: grid; grid-template-columns: repeat(3, minmax(80px, 1fr)); gap: 10px; max-width: 360px; }
    .motor-panel { display: grid; gap: 12px; max-width: 640px; }
    .motors { display: grid; grid-template-columns: repeat(2, minmax(120px, 1fr)); gap: 10px; }
    .motor { border: 1px solid #bbb; border-radius: 8px; padding: 12px; background: #fff; }
    .motor strong { display: block; margin-bottom: 6px; }
    .motor .direction { font-size: 20px; font-weight: 700; }
    .pins { display: grid; grid-template-columns: repeat(4, minmax(64px, 1fr)); gap: 8px; }
    .pin { border: 1px solid #bbb; border-radius: 8px; padding: 10px; background: #fff; text-align: center; }
    .pin.high { background: #d9f7df; border-color: #3c9b4f; }
    .pin.low { background: #f2f2f2; color: #555; }
    button, input { font: inherit; border-radius: 8px; border: 1px solid #999; padding: 12px; }
    button { background: #fff; cursor: pointer; }
    button:active { background: #dfe9ff; }
    .stop { background: #ffe8e8; }
    img { width: 100%; max-width: 640px; background: #222; display: block; }
    form { display: grid; gap: 10px; max-width: 420px; }
    code { background: #e8eaed; padding: 2px 5px; border-radius: 5px; }
    .color-panel { display: grid; grid-template-columns: 96px minmax(0, 1fr); gap: 14px; align-items: center; max-width: 420px; }
    .swatch { width: 96px; height: 96px; border: 1px solid #999; border-radius: 8px; background: #000; }
    .color-values { display: grid; gap: 4px; }
  </style>
</head>
<body>
<main>
  <h1>CameraDriver</h1>
  <p>Station IP: <code>)HTML";
  html += localIp;
  html += R"HTML(</code> Setup AP IP: <code>)HTML";
  html += apIp;
  html += R"HTML(</code></p>

  <section>
    <h2>Camera</h2>
    <p><img id="camera" alt="Camera stream"></p>
  </section>

  <section>
    <h2>Drive</h2>
    <div class="grid">
      <span></span><button onclick="cmd('forward')">Forward</button><span></span>
      <button onclick="cmd('left')">Left</button><button class="stop" onclick="cmd('stop')">Stop</button><button onclick="cmd('right')">Right</button>
      <span></span><button onclick="cmd('backward')">Backward</button><span></span>
    </div>
    <p id="status">Ready</p>
    <div class="grid">
      <button onclick="cmd('left-forward')">Left Fwd</button>
      <button onclick="cmd('left-backward')">Left Back</button>
      <button onclick="cmd('right-forward')">Right Fwd</button>
      <button onclick="cmd('right-backward')">Right Back</button>
    </div>
  </section>

  <section>
    <h2>Motor State</h2>
    <div class="motor-panel">
      <div class="motors">
        <div class="motor">
          <strong>Left motor</strong>
          <div id="left-direction" class="direction">Stopped</div>
          <div>DIR1 <span id="left-dir">LOW</span> / PWM1 <span id="left-pwm">0</span></div>
        </div>
        <div class="motor">
          <strong>Right motor</strong>
          <div id="right-direction" class="direction">Stopped</div>
          <div>DIR2 <span id="right-dir">LOW</span> / PWM2 <span id="right-pwm">0</span></div>
        </div>
      </div>
      <div class="pins">
        <div id="pin-dir1" class="pin low">DIR1<br>LOW</div>
        <div id="pin-pwm1" class="pin low">PWM1<br>0</div>
        <div id="pin-dir2" class="pin low">DIR2<br>LOW</div>
        <div id="pin-pwm2" class="pin low">PWM2<br>0</div>
      </div>
    </div>
  </section>

  <section>
    <h2>Color Sensor</h2>
    <div class="color-panel">
      <div id="color-swatch" class="swatch"></div>
      <div class="color-values">
        <div id="color-status">Not ready</div>
        <div>RGB <code id="color-rgb">0, 0, 0</code></div>
        <div>Raw <code id="color-raw">C 0 / R 0 / G 0 / B 0</code></div>
      </div>
    </div>
  </section>

  <section>
    <h2>Status LED</h2>
    <button onclick="cmd('headlights-on')">On</button>
    <button onclick="cmd('headlights-off')">Pattern</button>
  </section>

  <section>
    <h2>Wi-Fi</h2>
    <p>This XIAO ESP32S3 is running its own access point. Connect to <code>CameraDriver-Setup</code> and open <code>http://narbot.esp</code>.</p>
  </section>
</main>
<script>
document.getElementById('camera').src = 'http://' + location.hostname + ':81/stream';
async function cmd(name) {
  const response = await fetch('/' + name);
  document.getElementById('status').textContent = await response.text();
  refreshMotorState();
}
async function refreshMotorState() {
  const response = await fetch('/motor-state');
  const state = await response.json();
  document.getElementById('left-direction').textContent = state.left;
  document.getElementById('right-direction').textContent = state.right;
  setPin('dir1', state.dir1, state.dir1 ? 'HIGH' : 'LOW');
  setPin('pwm1', state.pwm1 > 0, state.pwm1);
  setPin('dir2', state.dir2, state.dir2 ? 'HIGH' : 'LOW');
  setPin('pwm2', state.pwm2 > 0, state.pwm2);
  document.getElementById('left-dir').textContent = state.dir1 ? 'HIGH' : 'LOW';
  document.getElementById('left-pwm').textContent = state.pwm1;
  document.getElementById('right-dir').textContent = state.dir2 ? 'HIGH' : 'LOW';
  document.getElementById('right-pwm').textContent = state.pwm2;
}
function setPin(name, high, value) {
  const pin = document.getElementById('pin-' + name);
  pin.className = 'pin ' + (high ? 'high' : 'low');
  pin.innerHTML = name.toUpperCase() + '<br>' + value;
}
async function refreshColorState() {
  const response = await fetch('/color-state');
  const state = await response.json();
  const swatch = document.getElementById('color-swatch');
  document.getElementById('color-status').textContent = state.ready ? 'Ready' : 'Not ready';
  document.getElementById('color-rgb').textContent = state.rgb.r + ', ' + state.rgb.g + ', ' + state.rgb.b;
  document.getElementById('color-raw').textContent = 'C ' + state.clear + ' / R ' + state.red + ' / G ' + state.green + ' / B ' + state.blue;
  swatch.style.backgroundColor = 'rgb(' + state.rgb.r + ',' + state.rgb.g + ',' + state.rgb.b + ')';
}
refreshMotorState();
refreshColorState();
setInterval(refreshColorState, 500);
document.addEventListener('keydown', (event) => {
  const key = event.key.toLowerCase();
  if (key === 'w') cmd('forward');
  if (key === 's') cmd('backward');
  if (key === 'a') cmd('left');
  if (key === 'd') cmd('right');
  if (key === 'x' || key === ' ') cmd('stop');
  if (key === 'o') cmd('headlights-on');
  if (key === 'p') cmd('headlights-off');
});
</script>
</body>
</html>
)HTML";

  server.send(200, "text/html", html);
}

void handleWifiSetupPage() {
  handleRoot();
}

void handleWifiSave() {
  server.send(400, "text/plain", "Station Wi-Fi setup is disabled; AP mode is always on.");
}

void handleWebCapture() {
  if (!cameraReady) {
    server.send(503, "text/plain", "Camera not ready");
    return;
  }

  camera_fb_t *fb = captureFreshFrame();
  if (fb == nullptr) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(fb->len);
  server.send(200, "image/jpeg", "");
  server.client().write(fb->buf, fb->len);

  esp_camera_fb_return(fb);
}

void startStreamServer() {
  streamServer.begin();
  Serial.println("Stream server started: http://narbot.esp:81/stream");
}

void updateStreamServer() {
  if (!cameraReady) {
    return;
  }

  if (!streamClient || !streamClient.connected()) {
    if (streamClient) {
      streamClient.stop();
    }

    WiFiClient candidate = streamServer.available();
    if (!candidate) {
      return;
    }

    streamClient = candidate;
    unsigned long startedAt = millis();
    while (streamClient.connected() && millis() - startedAt < 250) {
      while (streamClient.available()) {
        streamClient.read();
      }
    }

    sendStreamHeaders(streamClient);
    lastStreamFrameAt = 0;
    Serial.println("Stream client connected");
  }

  if (millis() - lastStreamFrameAt < STREAM_FRAME_INTERVAL_MS) {
    return;
  }

  lastStreamFrameAt = millis();
  sendStreamFrame(streamClient);
}

void sendStreamHeaders(WiFiClient &client) {
  client.print("HTTP/1.1 200 OK\r\n");
  client.print("Content-Type: multipart/x-mixed-replace; boundary=frame\r\n");
  client.print("Cache-Control: no-store\r\n");
  client.print("Access-Control-Allow-Origin: *\r\n");
  client.print("\r\n");
}

void sendStreamFrame(WiFiClient &client) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb == nullptr) {
    Serial.println("Stream capture failed");
    return;
  }

  client.printf("--frame\r\n");
  client.printf("Content-Type: image/jpeg\r\n");
  client.printf("Content-Length: %u\r\n\r\n", fb->len);

  size_t written = client.write(fb->buf, fb->len);
  client.print("\r\n");

  if (written != fb->len) {
    Serial.println("Stream client disconnected");
    client.stop();
  }

  esp_camera_fb_return(fb);
}

void handleMotorState() {
  String json = "{";
  json += "\"left\":\"";
  json += motorDirectionName(leftMotorState);
  json += "\",\"right\":\"";
  json += motorDirectionName(rightMotorState);
  json += "\",\"dir1\":";
  json += leftMotorState == MOTOR_BACKWARD ? "true" : "false";
  json += ",\"pwm1\":";
  json += leftMotorPwm;
  json += ",\"dir2\":";
  json += rightMotorState == MOTOR_BACKWARD ? "true" : "false";
  json += ",\"pwm2\":";
  json += rightMotorPwm;
  json += "}";

  server.send(200, "application/json", json);
}

void handleColorState() {
  String json = "{";
  json += "\"ready\":";
  json += colorSensorReady && latestColor.available ? "true" : "false";
  json += ",\"id\":";
  json += colorSensorId;
  json += ",\"clear\":";
  json += latestColor.clear;
  json += ",\"red\":";
  json += latestColor.red;
  json += ",\"green\":";
  json += latestColor.green;
  json += ",\"blue\":";
  json += latestColor.blue;
  json += ",\"rgb\":{\"r\":";
  json += latestColor.displayRed;
  json += ",\"g\":";
  json += latestColor.displayGreen;
  json += ",\"b\":";
  json += latestColor.displayBlue;
  json += "}}";

  server.send(200, "application/json", json);
}

void sendPlainText(const String &message) {
  server.send(200, "text/plain", message);
}

void printNetworkStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Web control: http://");
    Serial.println(WiFi.localIP());
  }

  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    Serial.print("Setup AP page: http://");
    Serial.println(WiFi.softAPIP());
  }
}

void requestRestart() {
  restartRequested = true;
  restartAt = millis() + 1000;
}

void driveForward() {
  setStatusLedPattern(FORWARD_LED_PATTERN);

  setMotorState(MOTOR_FORWARD, MOTOR_FORWARD);
}

void driveBackward() {
  setStatusLedPattern(BACKWARD_LED_PATTERN);

  setMotorState(MOTOR_BACKWARD, MOTOR_BACKWARD);
}

void turnLeft() {
  setStatusLedPattern(LEFT_LED_PATTERN);

  setMotorState(MOTOR_BACKWARD, MOTOR_FORWARD);
}

void turnRight() {
  setStatusLedPattern(RIGHT_LED_PATTERN);

  setMotorState(MOTOR_FORWARD, MOTOR_BACKWARD);
}

void testLeftForward() {
  setStatusLedPattern(FORWARD_LED_PATTERN);

  setMotorState(MOTOR_FORWARD, MOTOR_STOPPED);
}

void testLeftBackward() {
  setStatusLedPattern(BACKWARD_LED_PATTERN);

  setMotorState(MOTOR_BACKWARD, MOTOR_STOPPED);
}

void testRightForward() {
  setStatusLedPattern(FORWARD_LED_PATTERN);

  setMotorState(MOTOR_STOPPED, MOTOR_FORWARD);
}

void testRightBackward() {
  setStatusLedPattern(BACKWARD_LED_PATTERN);

  setMotorState(MOTOR_STOPPED, MOTOR_BACKWARD);
}

void stopMotors() {
  setStatusLedPattern(STOP_LED_PATTERN);

  setMotorState(MOTOR_STOPPED, MOTOR_STOPPED);
}

void setMotorState(MotorDirection leftDirection, MotorDirection rightDirection) {
  leftMotorState = leftDirection;
  rightMotorState = rightDirection;
  leftMotorPwm = leftDirection == MOTOR_STOPPED ? 0 : MOTOR_SPEED;
  rightMotorPwm = rightDirection == MOTOR_STOPPED ? 0 : MOTOR_SPEED;

  writeMotor(LEFT_DIR_PIN, LEFT_PWM_CHANNEL, leftDirection, leftMotorPwm);
  writeMotor(RIGHT_DIR_PIN, RIGHT_PWM_CHANNEL, rightDirection, rightMotorPwm);
}

void writeMotor(int dirPin, int pwmChannel, MotorDirection direction, int speed) {
  digitalWrite(dirPin, direction == MOTOR_BACKWARD ? HIGH : LOW);
  ledcWrite(pwmChannel, speed);
}

const char *motorDirectionName(MotorDirection direction) {
  if (direction == MOTOR_FORWARD) {
    return "Forward";
  }

  if (direction == MOTOR_BACKWARD) {
    return "Backward";
  }

  return "Stopped";
}

void setStatusLedPattern(const LedPattern &pattern) {
  currentLedPattern = &pattern;
  currentLedStep = 0;
  ledStepStartedAt = millis();
  writeStatusLed(currentLedPattern->steps[currentLedStep].on);
}

void updateStatusLed() {
  if (headlightsEnabled) {
    return;
  }

  unsigned long now = millis();
  const LedPatternStep &step = currentLedPattern->steps[currentLedStep];

  if (now - ledStepStartedAt < step.durationMs) {
    return;
  }

  currentLedStep = (currentLedStep + 1) % currentLedPattern->stepCount;
  ledStepStartedAt = now;
  writeStatusLed(currentLedPattern->steps[currentLedStep].on);
}

void setHeadlights(bool enabled) {
  headlightsEnabled = enabled;
  digitalWrite(STATUS_LED, enabled ? HIGH : LOW);

  if (!enabled) {
    writeStatusLed(currentLedPattern->steps[currentLedStep].on);
  }
}

void writeStatusLed(bool on) {
  if (headlightsEnabled) {
    digitalWrite(STATUS_LED, HIGH);
    return;
  }

  digitalWrite(STATUS_LED, on ? HIGH : LOW);
}
