Create a minimal PlatformIO Arduino C++ project for a two-ESP32 robot camera setup.

Hardware:
- Both boards are Seeed Studio XIAO ESP32S3 Sense boards with camera modules.
- Use PlatformIO with framework = arduino.
- Use board = seeed_xiao_esp32s3.
- Use Serial logging at 115200 baud.

Goal:
Set up one codebase that can be built in two modes:
1. MAIN_ESP
2. FRONT_ESP

Use build flags in platformio.ini:
- env:main_esp with -D MAIN_ESP
- env:front_esp with -D FRONT_ESP

Do not overcomplicate the project yet. Avoid FreeRTOS task architecture for now unless absolutely necessary. Use simple Arduino setup/loop plus non-blocking style where possible. The goal is to get Wi-Fi, web server, camera streaming, and logs working first.

Required files:
- platformio.ini
- src/main.cpp
- src/config.h
- src/camera_pins.h
- src/camera_setup.h / .cpp
- src/web_server.h / .cpp
- src/wifi_setup.h / .cpp

Functionality:

MAIN_ESP mode:
- Start Wi-Fi Access Point.
- AP SSID: "SMARS_MAIN"
- AP password: "smars1234"
- Use AP IP 192.168.4.1.
- Start a web server on port 80.
- Initialize the local XIAO ESP32S3 Sense camera.
- Expose endpoints:
  - GET /              -> main HTML dashboard
  - GET /status        -> plain text or JSON status
  - GET /jpg           -> one JPEG frame from main ESP camera
  - GET /stream        -> MJPEG stream from main ESP camera
- The dashboard page should show:
  - Title: "SMARS Robot Dashboard"
  - Main ESP camera feed using <img src="/stream">
  - Front ESP camera feed using <img src="http://192.168.4.2/stream">
  - Basic status text
- Log to Serial:
  - boot mode
  - Wi-Fi AP started
  - AP IP address
  - camera init success/failure
  - web server started
  - every incoming request path if easy to do

FRONT_ESP mode:
- Connect to the MAIN_ESP access point as Wi-Fi station.
- SSID: "SMARS_MAIN"
- password: "smars1234"
- Try to use static IP 192.168.4.2, gateway 192.168.4.1, subnet 255.255.255.0.
- Start a web server on port 80.
- Initialize the local XIAO ESP32S3 Sense camera.
- Expose endpoints:
  - GET /status        -> plain text or JSON status
  - GET /jpg           -> one JPEG frame
  - GET /stream        -> MJPEG stream
- Log to Serial:
  - boot mode
  - Wi-Fi connection attempts
  - assigned IP
  - camera init success/failure
  - web server started
  - every incoming request path if easy to do

Camera requirements:
- Use esp_camera.h.
- Configure the camera for Seeed Studio XIAO ESP32S3 Sense.
- Use PSRAM if available.
- Start with low resolution for reliability:
  - frame_size = FRAMESIZE_QVGA
  - jpeg_quality around 12
  - fb_count = 1 or 2 depending on PSRAM
- Include clear comments where the XIAO ESP32S3 Sense camera pin mapping is defined.
- If exact pin mapping is uncertain, make it isolated in camera_pins.h and add a comment saying it must match the Seeed XIAO ESP32S3 Sense camera example.

Web server:
- Prefer the standard ESP32 Arduino WebServer library for simplicity.
- Implement a basic MJPEG stream endpoint similar to common ESP32 CameraWebServer examples.
- Keep the code readable and beginner-friendly.
- Avoid async web server libraries for the first version.
- Avoid websockets for now.
- Avoid OTA for now.
- Avoid motor code for now.
- Avoid QR code and color detection for now.

Code quality:
- Split code into small files but do not over-engineer.
- Use functions like:
  - setupWifi()
  - setupCamera()
  - setupWebServer()
  - handleWebServer()
  - logBootInfo()
- Use #ifdef MAIN_ESP and #ifdef FRONT_ESP where needed.
- Make it compile in PlatformIO.
- Include comments explaining how to build and upload each environment:
  - pio run -e main_esp -t upload
  - pio run -e front_esp -t upload
  - pio device monitor

Important:
- Do not create a complex robotics framework yet.
- Do not add FreeRTOS tasks yet.
- Do not add motor control yet.
- Do not add QR decoding yet.
- First milestone is:
  1. Main ESP starts AP.
  2. Main ESP dashboard opens at http://192.168.4.1/
  3. Main ESP camera stream is visible.
  4. Front ESP connects to AP.
  5. Front ESP stream is visible from the main dashboard.
