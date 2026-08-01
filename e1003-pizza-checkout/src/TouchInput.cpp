#include "TouchInput.h"

#include <ArduinoJson.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <WiFi.h>

#include "AppState.h"

namespace {
constexpr int TOUCH_SDA = 19;
constexpr int TOUCH_SCL = 20;
constexpr int TOUCH_INT = 2;
constexpr int TOUCH_RESET = 48;
constexpr uint32_t DEBOUNCE_MS = 800;

uint32_t g_lastEmitMs = 0;
String g_lastButtonId;

bool pointInRect(int x, int y, const ButtonRect& rect) {
  return x >= rect.x && x < (rect.x + rect.w) && y >= rect.y && y < (rect.y + rect.h);
}

void postCallback(const TouchEvent& event) {
  if (g_deviceConfig.callbackUrl.isEmpty() || WiFi.status() != WL_CONNECTED) {
    return;
  }

  JsonDocument doc;
  doc["deviceId"] = g_deviceConfig.deviceId;
  doc["event"] = event.event;
  doc["screenId"] = event.screenId;
  doc["buttonId"] = event.buttonId;
  doc["label"] = event.label;
  doc["timestampMs"] = event.timestampMs;

  String payload;
  serializeJson(doc, payload);

  HTTPClient client;
  client.begin(g_deviceConfig.callbackUrl);
  client.addHeader("Content-Type", "application/json");
  int status = client.POST(payload);
  Serial.printf("Callback status: %d\n", status);
  client.end();
}

void handleTouchPoint(int x, int y) {
  for (const ButtonRect& rect : g_buttonRects) {
    if (!pointInRect(x, y, rect)) {
      continue;
    }

    const uint32_t now = millis();
    if (rect.id == g_lastButtonId && (now - g_lastEmitMs) < DEBOUNCE_MS) {
      return;
    }

    g_lastButtonId = rect.id;
    g_lastEmitMs = now;

    g_latestTouchEvent.event = "button_pressed";
    g_latestTouchEvent.screenId = g_rightPanelState.screenId;
    g_latestTouchEvent.buttonId = rect.id;
    g_latestTouchEvent.label = rect.label;
    g_latestTouchEvent.timestampMs = now;

    postCallback(g_latestTouchEvent);
    return;
  }
}
}  // namespace

namespace TouchInput {
bool begin() {
  pinMode(TOUCH_INT, INPUT_PULLUP);
  pinMode(TOUCH_RESET, OUTPUT);
  digitalWrite(TOUCH_RESET, HIGH);
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  Serial.println("TouchInput initialized (GT911 pins configured)");
  return true;
}

void poll() {
  // TODO: Replace this placeholder with actual GT911 polling based on the
  // Seeed E1003 touch example once the exact library/API is finalized.
  // For now, developers can simulate touches over serial:
  // t 1200 900
  if (!Serial.available()) {
    return;
  }

  const String line = Serial.readStringUntil('\n');
  if (!line.startsWith("t ")) {
    return;
  }

  int firstSpace = line.indexOf(' ');
  int secondSpace = line.indexOf(' ', firstSpace + 1);
  if (firstSpace < 0 || secondSpace < 0) {
    return;
  }

  int x = line.substring(firstSpace + 1, secondSpace).toInt();
  int y = line.substring(secondSpace + 1).toInt();
  Serial.printf("Simulated touch x=%d y=%d\n", x, y);
  handleTouchPoint(x, y);
}
}  // namespace
