#pragma once

#include <ArduinoJson.h>
#include <WebServer.h>

class ApiServer {
 public:
  ApiServer();
  void begin();
  void handleClient();

 private:
  WebServer server_;

  bool validateToken();
  bool parseJsonBody(JsonDocument& doc);
  void sendJson(int statusCode, const JsonDocument& doc);

  void handleHealth();
  void handleState();
  void handleLatestEvent();
  void handleConfig();
  void handleBasket();
  void handleRightPanel();
  void handleCustomisation();
  void handleScreen();
  void handleRefresh();
  void handleClear();
};
