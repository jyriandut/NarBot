#include "web_server.h"

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_camera.h>

#include "color_sensor.h"
#include "config.h"
#include "distance_sensor.h"
#include "motor_driver.h"
#include "obstacle_safety.h"

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
  json += ",\"targetSpeed\":";
  json += state.targetSpeed;
  json += "}";
  return json;
}

void sendDriveCommandResponse(const char *message) {
  server.send(200, "application/json", String("{\"message\":\"") + message + "\",\"state\":" + motorStateJson() + "}");
}

String safetyJson() {
  ObstacleSafetyState safety = getObstacleSafetyState();
  String json = "{";
  json += "\"enabled\":";
  json += safety.enabled ? "true" : "false";
  json += ",\"frontReachable\":";
  json += safety.frontReachable ? "true" : "false";
  json += ",\"distanceValid\":";
  json += safety.distanceValid ? "true" : "false";
  json += ",\"blocked\":";
  json += safety.blocked ? "true" : "false";
  json += ",\"distanceMm\":";
  json += safety.distanceMm;
  json += ",\"lastCheckAt\":";
  json += safety.lastCheckAt;
  json += "}";
  return json;
}

String colorJson() {
  ColorSensorState color = getColorSensorState();
  String json = "{";
  json += "\"ready\":";
  json += color.ready ? "true" : "false";
  json += ",\"available\":";
  json += color.available ? "true" : "false";
  json += ",\"id\":";
  json += String(color.id);
  json += ",\"clear\":";
  json += color.clear;
  json += ",\"red\":";
  json += color.red;
  json += ",\"green\":";
  json += color.green;
  json += ",\"blue\":";
  json += color.blue;
  json += ",\"displayRed\":";
  json += String(color.displayRed);
  json += ",\"displayGreen\":";
  json += String(color.displayGreen);
  json += ",\"displayBlue\":";
  json += String(color.displayBlue);
  json += ",\"targetDetected\":";
  json += color.targetDetected ? "true" : "false";
  json += ",\"lastReadAt\":";
  json += color.lastReadAt;
  json += "}";
  return json;
}

#endif

#ifdef FRONT_ESP
String distanceJson() {
  DistanceSensorState distance = getDistanceSensorState();
  String json = "{";
  json += "\"ready\":";
  json += distance.ready ? "true" : "false";
  json += ",\"valid\":";
  json += distance.valid ? "true" : "false";
  json += ",\"distanceMm\":";
  json += distance.distanceMm;
  json += ",\"rangeStatus\":";
  json += distance.rangeStatus;
  json += ",\"lastReadAt\":";
  json += distance.lastReadAt;
  json += "}";
  return json;
}

void sendCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}
#endif

void handleRoot() {
  logRequest();
#ifdef MAIN_ESP
  static const char html[] PROGMEM = R"HTML(
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
    .color-row { display: flex; gap: 10px; align-items: center; }
    .color-swatch { width: 28px; height: 28px; border: 1px solid #555; border-radius: 4px; background: #000; }
    .speed-control { display: grid; grid-template-columns: auto minmax(160px, 280px) 44px; gap: 10px; align-items: center; margin: 0 0 12px; }
    input[type="range"] { width: 100%; }
  </style>
</head>
<body>
  <main>
    <h1>SMARS Robot Dashboard</h1>
    <div class="status">Main ESP at 192.168.4.1, Front ESP expected at 192.168.4.10</div>
    <div class="feeds">
      <section>
        <h2>Main ESP Camera</h2>
        <img id="main-camera" alt="Main ESP camera stream">
        <div class="motor-state">
          <div class="color-row"><span>Color</span><span id="color-swatch" class="color-swatch"></span><code id="color-rgb">--</code></div>
          <div>Color stop <code id="color-stop">--</code></div>
          <div>Raw <code id="color-raw">--</code></div>
        </div>
      </section>
      <section>
        <h2>Front ESP Camera</h2>
        <img src="http://192.168.4.10:81/stream" alt="Front ESP camera stream">
        <div class="motor-state">
          <div>Distance <code id="front-distance">--</code></div>
        </div>
      </section>
    </div>
    <section class="drive">
      <h2>Drive</h2>
      <label class="speed-control">
        <span>Speed</span>
        <input id="speed-slider" type="range" min="0" max="255" value="180" oninput="previewSpeed()" onchange="setSpeed()">
        <code id="speed-value">180</code>
      </label>
      <div class="drive-grid">
        <span></span><button onclick="drive('forward')">Forward</button><span></span>
        <button onclick="drive('left')">Left</button><button class="stop" onclick="drive('stop')">Stop</button><button onclick="drive('right')">Right</button>
        <span></span><button onclick="drive('backward')">Backward</button><span></span>
      </div>
      <div class="motor-state">
        <div id="drive-status">Ready</div>
        <div>Safety <code id="safety-state">--</code></div>
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
      document.getElementById('speed-slider').value = state.targetSpeed;
      document.getElementById('speed-value').textContent = state.targetSpeed;
    }

    function previewSpeed() {
      document.getElementById('speed-value').textContent = document.getElementById('speed-slider').value;
    }

    async function setSpeed() {
      const value = document.getElementById('speed-slider').value;
      try {
        const response = await fetch('/drive/speed?value=' + encodeURIComponent(value));
        if (!response.ok) throw new Error('HTTP ' + response.status);
        const payload = await response.json();
        document.getElementById('drive-status').textContent = payload.message;
        renderMotorState(payload.state);
      } catch (error) {
        document.getElementById('drive-status').textContent = 'Speed command failed: ' + error.message;
      }
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

    async function publishSafetyDistance(distance) {
      try {
        const valid = distance.ready && distance.valid ? '1' : '0';
        await fetch('/safety-distance?valid=' + valid + '&distance=' + encodeURIComponent(distance.distanceMm || 0));
      } catch (error) {
      }
    }

    async function refreshFrontDistance() {
      try {
        const response = await fetch('http://192.168.4.10/distance');
        if (!response.ok) throw new Error('HTTP ' + response.status);
        const distance = await response.json();
        document.getElementById('front-distance').textContent =
          distance.ready && distance.valid ? distance.distanceMm + ' mm' : 'not ready';
        publishSafetyDistance(distance);
      } catch (error) {
        document.getElementById('front-distance').textContent = 'unavailable: ' + error.message;
      }
    }

    async function refreshSafetyState() {
      try {
        const response = await fetch('/safety');
        if (!response.ok) throw new Error('HTTP ' + response.status);
        const safety = await response.json();
        if (!safety.enabled) {
          document.getElementById('safety-state').textContent = 'off';
        } else if (!safety.frontReachable) {
          document.getElementById('safety-state').textContent = 'front unavailable';
        } else if (safety.blocked) {
          document.getElementById('safety-state').textContent = 'blocked at ' + safety.distanceMm + ' mm';
        } else {
          document.getElementById('safety-state').textContent = safety.distanceMm + ' mm';
        }
      } catch (error) {
        document.getElementById('safety-state').textContent = 'unavailable';
      }
    }

    async function refreshMainColor() {
      try {
        const response = await fetch('/color');
        if (!response.ok) throw new Error('HTTP ' + response.status);
        const color = await response.json();
        if (!color.ready) {
          document.getElementById('color-rgb').textContent = 'not found';
          document.getElementById('color-stop').textContent = 'off';
          document.getElementById('color-raw').textContent = '--';
          document.getElementById('color-swatch').style.background = '#000';
          return;
        }
        if (!color.available) {
          document.getElementById('color-rgb').textContent = 'waiting';
          return;
        }

        const rgb = 'rgb(' + color.displayRed + ', ' + color.displayGreen + ', ' + color.displayBlue + ')';
        document.getElementById('color-swatch').style.background = rgb;
        document.getElementById('color-rgb').textContent = rgb;
        document.getElementById('color-stop').textContent = color.targetDetected ? 'matched' : 'clear';
        document.getElementById('color-raw').textContent =
          'C ' + color.clear + ' / R ' + color.red + ' / G ' + color.green + ' / B ' + color.blue;
      } catch (error) {
        document.getElementById('color-rgb').textContent = 'unavailable';
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
    refreshFrontDistance();
    refreshSafetyState();
    refreshMainColor();
    setInterval(refreshFrontDistance, 300);
    setInterval(refreshSafetyState, 300);
    setInterval(refreshMainColor, 500);
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

#ifdef FRONT_ESP
void handleDistance() {
  logRequest();
  sendCorsHeaders();
  server.send(200, "application/json", distanceJson());
}

void handleDistanceOptions() {
  logRequest();
  sendCorsHeaders();
  server.send(204);
}
#endif

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

void handleDriveSpeed() {
  logRequest();
  if (!server.hasArg("value")) {
    server.send(400, "application/json", "{\"error\":\"Missing value\"}");
    return;
  }

  setMotorTargetSpeed(server.arg("value").toInt());
  sendDriveCommandResponse("Speed updated");
}

void handleSafety() {
  logRequest();
  server.send(200, "application/json", safetyJson());
}

void handleSafetyDistance() {
  logRequest();
  bool valid = server.hasArg("valid") && server.arg("valid") == "1";
  uint16_t distanceMm = server.hasArg("distance") ? server.arg("distance").toInt() : 0;
  updateFrontDistanceForSafety(valid, distanceMm);
  server.send(204);
}

void handleColor() {
  logRequest();
  server.send(200, "application/json", colorJson());
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
  server.on("/drive/speed", HTTP_GET, handleDriveSpeed);
  server.on("/safety", HTTP_GET, handleSafety);
  server.on("/safety-distance", HTTP_GET, handleSafetyDistance);
  server.on("/color", HTTP_GET, handleColor);
#endif
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/jpg", HTTP_GET, handleJpg);
  server.on("/stream", HTTP_GET, handleStream);
#ifdef FRONT_ESP
  server.on("/distance", HTTP_GET, handleDistance);
  server.on("/distance", HTTP_OPTIONS, handleDistanceOptions);
#endif
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
