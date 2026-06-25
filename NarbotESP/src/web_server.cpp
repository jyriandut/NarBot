#include "web_server.h"

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_camera.h>

#include "config.h"
#include "motor_driver.h"

namespace {

WebServer server(WEB_SERVER_PORT);
WiFiServer streamServer(81);
WiFiClient streamClient;

const char *STREAM_CONTENT_TYPE = "multipart/x-mixed-replace; boundary=frame";
const char *STREAM_BOUNDARY = "\r\n--frame\r\n";
const char *STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
constexpr unsigned long STREAM_FRAME_INTERVAL_MS = 100;

unsigned long lastStreamFrameAt = 0;

void logRequest() {
  Serial.print("HTTP ");
  Serial.println(server.uri());
}

String statusJson() {
  String json = "{";
#ifdef MAIN_ESP
  json += "\"mode\":\"MAIN_ESP\",";
  json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"stations\":" + String(WiFi.softAPgetStationNum());
#else
  json += "\"mode\":\"FRONT_ESP\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"rssi\":" + String(WiFi.RSSI());
#endif
  json += "}";
  return json;
}

#ifdef MAIN_ESP
String motorStateJson() {
  MotorDriverState state = getMotorDriverState();
  String json = "{";
  json += "\"left\":\"";
  json += motorDirectionName(state.leftDirection);
  json += "\",\"right\":\"";
  json += motorDirectionName(state.rightDirection);
  json += "\",\"leftPwm\":";
  json += state.leftPwm;
  json += ",\"rightPwm\":";
  json += state.rightPwm;
  json += "}";
  return json;
}

void sendDriveCommandResponse(const char *message) {
  server.send(200, "application/json", String("{\"message\":\"") + message + "\",\"state\":" + motorStateJson() + "}");
}
#endif

void handleRoot() {
  logRequest();
#ifdef MAIN_ESP
  const char html[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>SMARS Robot Dashboard</title>
  <style>
    body { margin: 0; font-family: system-ui, sans-serif; background: #111; color: #f4f4f4; }
    main { max-width: 960px; margin: 0 auto; padding: 20px; }
    h1 { margin: 0 0 12px; font-size: 28px; }
    .status { margin-bottom: 16px; color: #ccc; }
    .feeds { display: grid; gap: 16px; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); }
    section { background: #1d1d1d; border: 1px solid #333; border-radius: 8px; padding: 12px; }
    h2 { margin: 0 0 10px; font-size: 18px; }
    img { display: block; width: 100%; height: auto; background: #000; }
    .drive { margin-top: 16px; }
    .drive-grid { display: grid; grid-template-columns: repeat(3, minmax(72px, 1fr)); gap: 10px; max-width: 360px; }
    .drive-grid span { min-height: 44px; }
    button { min-height: 44px; border: 1px solid #555; border-radius: 8px; background: #2a2a2a; color: #f4f4f4; font: inherit; cursor: pointer; }
    button:active { background: #444; }
    button.stop { background: #4b2020; border-color: #704040; }
    .motor-state { display: grid; gap: 6px; margin-top: 12px; color: #ddd; }
    .motor-state code { background: #101010; border: 1px solid #333; border-radius: 5px; padding: 2px 5px; }
  </style>
</head>
<body>
  <main>
    <h1>SMARS Robot Dashboard</h1>
    <div class="status">Main ESP at 192.168.4.1, Front ESP expected at 192.168.4.2</div>
    <div class="feeds">
      <section>
        <h2>Main ESP Camera</h2>
        <img id="main-camera" alt="Main ESP camera stream">
      </section>
      <section>
        <h2>Front ESP Camera</h2>
        <img src="http://192.168.4.2:81/stream" alt="Front ESP camera stream">
      </section>
    </div>
    <section class="drive">
      <h2>Drive</h2>
      <div class="drive-grid">
        <span></span><button onclick="drive('forward')">Forward</button><span></span>
        <button onclick="drive('left')">Left</button><button class="stop" onclick="drive('stop')">Stop</button><button onclick="drive('right')">Right</button>
        <span></span><button onclick="drive('backward')">Backward</button><span></span>
      </div>
      <div class="motor-state">
        <div id="drive-status">Ready</div>
        <div>Left <code id="left-state">Stopped / 0</code></div>
        <div>Right <code id="right-state">Stopped / 0</code></div>
      </div>
    </section>
  </main>
  <script>
    document.getElementById('main-camera').src = 'http://' + location.hostname + ':81/stream';

    function renderMotorState(state) {
      document.getElementById('left-state').textContent = state.left + ' / ' + state.leftPwm;
      document.getElementById('right-state').textContent = state.right + ' / ' + state.rightPwm;
    }

    async function refreshMotorState() {
      try {
        const response = await fetch('/drive/state');
        if (!response.ok) throw new Error('HTTP ' + response.status);
        renderMotorState(await response.json());
      } catch (error) {
        document.getElementById('drive-status').textContent = 'Drive state failed: ' + error.message;
      }
    }

    async function drive(command) {
      try {
        const response = await fetch('/drive/' + command);
        if (!response.ok) throw new Error('HTTP ' + response.status);
        const payload = await response.json();
        document.getElementById('drive-status').textContent = payload.message;
        renderMotorState(payload.state);
      } catch (error) {
        document.getElementById('drive-status').textContent = 'Drive command failed: ' + error.message;
      }
    }

    document.addEventListener('keydown', (event) => {
      const key = event.key.toLowerCase();
      if (['w', 'a', 's', 'd', 'x', ' '].includes(key)) {
        event.preventDefault();
      }
      if (key === 'w') drive('forward');
      if (key === 's') drive('backward');
      if (key === 'a') drive('left');
      if (key === 'd') drive('right');
      if (key === 'x' || key === ' ') drive('stop');
    });

    refreshMotorState();
  </script>
</body>
</html>
)HTML";
  server.send_P(200, "text/html", html);
#else
  server.send(404, "text/plain", "Front ESP has no dashboard. Use /status, /jpg, or /stream.");
#endif
}

void handleStatus() {
  logRequest();
  server.send(200, "application/json", statusJson());
}

#ifdef MAIN_ESP
void handleDriveState() {
  logRequest();
  server.send(200, "application/json", motorStateJson());
}

void handleDriveForward() {
  logRequest();
  driveForward();
  sendDriveCommandResponse("Forward");
}

void handleDriveBackward() {
  logRequest();
  driveBackward();
  sendDriveCommandResponse("Backward");
}

void handleDriveLeft() {
  logRequest();
  turnLeft();
  sendDriveCommandResponse("Left");
}

void handleDriveRight() {
  logRequest();
  turnRight();
  sendDriveCommandResponse("Right");
}

void handleDriveStop() {
  logRequest();
  stopMotors();
  sendDriveCommandResponse("Stop");
}
#endif

void handleJpg() {
  logRequest();
  camera_fb_t *frame = esp_camera_fb_get();
  if (frame == nullptr) {
    server.send(503, "text/plain", "Camera capture failed");
    return;
  }

  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.setContentLength(frame->len);
  server.send(200, "image/jpeg", "");
  server.client().write(frame->buf, frame->len);
  esp_camera_fb_return(frame);
}

void handleStream() {
  logRequest();
  server.send(200, "text/plain", "Camera stream is available on port 81: /stream");
}

void handleNotFound() {
  logRequest();
  server.send(404, "text/plain", "Not found");
}

void sendStreamHeaders(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.print("Content-Type: ");
  client.println(STREAM_CONTENT_TYPE);
  client.println("Cache-Control: no-cache");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
}

void sendStreamFrame(WiFiClient &client) {
  camera_fb_t *frame = esp_camera_fb_get();
  if (frame == nullptr) {
    Serial.println("Camera capture failed during stream");
    return;
  }

  char partHeader[64];
  size_t headerLength = snprintf(partHeader, sizeof(partHeader), STREAM_PART, frame->len);

  bool writeOk = client.write(reinterpret_cast<const uint8_t *>(STREAM_BOUNDARY), strlen(STREAM_BOUNDARY)) == strlen(STREAM_BOUNDARY) &&
                 client.write(reinterpret_cast<const uint8_t *>(partHeader), headerLength) == headerLength &&
                 client.write(frame->buf, frame->len) == frame->len;

  esp_camera_fb_return(frame);

  if (!writeOk) {
    Serial.println("Stream client disconnected");
    client.stop();
  }
}

void handleStreamServer() {
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
    while (streamClient.connected() && millis() - startedAt < 150) {
      while (streamClient.available()) {
        streamClient.read();
      }
      delay(1);
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

}  // namespace

void setupWebServer() {
#ifdef MAIN_ESP
  server.on("/", HTTP_GET, handleRoot);
  server.on("/drive/state", HTTP_GET, handleDriveState);
  server.on("/drive/forward", HTTP_GET, handleDriveForward);
  server.on("/drive/backward", HTTP_GET, handleDriveBackward);
  server.on("/drive/left", HTTP_GET, handleDriveLeft);
  server.on("/drive/right", HTTP_GET, handleDriveRight);
  server.on("/drive/stop", HTTP_GET, handleDriveStop);
#endif
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/jpg", HTTP_GET, handleJpg);
  server.on("/stream", HTTP_GET, handleStream);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web server started");

  streamServer.begin();
  Serial.println("Stream server started on port 81");
}

void handleWebServer() {
  server.handleClient();
  handleStreamServer();
}
