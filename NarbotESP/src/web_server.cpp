#include "web_server.h"

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_camera.h>

#include "config.h"

namespace {

WebServer server(WEB_SERVER_PORT);

const char *STREAM_CONTENT_TYPE = "multipart/x-mixed-replace; boundary=frame";
const char *STREAM_BOUNDARY = "\r\n--frame\r\n";
const char *STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

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
  </style>
</head>
<body>
  <main>
    <h1>SMARS Robot Dashboard</h1>
    <div class="status">Main ESP at 192.168.4.1, Front ESP expected at 192.168.4.2</div>
    <div class="feeds">
      <section>
        <h2>Main ESP Camera</h2>
        <img src="/stream" alt="Main ESP camera stream">
      </section>
      <section>
        <h2>Front ESP Camera</h2>
        <img src="http://192.168.4.2/stream" alt="Front ESP camera stream">
      </section>
    </div>
  </main>
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
  WiFiClient client = server.client();

  client.println("HTTP/1.1 200 OK");
  client.print("Content-Type: ");
  client.println(STREAM_CONTENT_TYPE);
  client.println("Cache-Control: no-cache");
  client.println("Connection: close");
  client.println();

  while (client.connected()) {
    camera_fb_t *frame = esp_camera_fb_get();
    if (frame == nullptr) {
      Serial.println("Camera capture failed during stream");
      break;
    }

    char partHeader[64];
    size_t headerLength = snprintf(partHeader, sizeof(partHeader), STREAM_PART, frame->len);

    if (client.write(reinterpret_cast<const uint8_t *>(STREAM_BOUNDARY), strlen(STREAM_BOUNDARY)) != strlen(STREAM_BOUNDARY) ||
        client.write(reinterpret_cast<const uint8_t *>(partHeader), headerLength) != headerLength ||
        client.write(frame->buf, frame->len) != frame->len) {
      esp_camera_fb_return(frame);
      break;
    }

    esp_camera_fb_return(frame);
    delay(10);
  }
}

void handleNotFound() {
  logRequest();
  server.send(404, "text/plain", "Not found");
}

}  // namespace

void setupWebServer() {
#ifdef MAIN_ESP
  server.on("/", HTTP_GET, handleRoot);
#endif
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/jpg", HTTP_GET, handleJpg);
  server.on("/stream", HTTP_GET, handleStream);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web server started");
}

void handleWebServer() {
  server.handleClient();
}
