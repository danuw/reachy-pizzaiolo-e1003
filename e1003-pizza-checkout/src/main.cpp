#include <WiFi.h>

#include "ApiServer.h"
#include "AppState.h"
#include "Renderer.h"
#include "TouchInput.h"
#include "driver.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

namespace {
ApiServer g_apiServer;

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  uint32_t startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection timeout. API will be unavailable until connected.");
  }
}

void ensureWifiConnected() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  static uint32_t lastAttempt = 0;
  if (millis() - lastAttempt < 5000) {
    return;
  }

  lastAttempt = millis();
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Starting E1003 pizza checkout firmware");

  resetToWelcomeState();
  g_deviceConfig.apiToken = API_TOKEN;

  connectWifi();
  Renderer::begin();
  TouchInput::begin();
  g_apiServer.begin();
  Renderer::renderAll();
}

void loop() {
  ensureWifiConnected();
  g_apiServer.handleClient();
  TouchInput::poll();
  delay(10);
}
