#include "ApiServer.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "AppState.h"
#include "Renderer.h"

namespace {
bool readBasketFromJson(const JsonVariantConst& src, BasketState& basket) {
  if (!src.is<JsonObjectConst>()) {
    return false;
  }

  basket.title = src["title"] | basket.title;
  basket.currency = src["currency"] | basket.currency;
  basket.status = src["status"] | "";
  basket.subtotal = src["subtotal"] | 0.0f;
  basket.discount = src["discount"] | 0.0f;
  basket.delivery = src["delivery"] | 0.0f;
  basket.total = src["total"] | 0.0f;

  basket.items.clear();
  JsonArrayConst items = src["items"].as<JsonArrayConst>();
  for (JsonVariantConst itemVar : items) {
    if ((int)basket.items.size() >= 20) {
      break;
    }
    BasketItem item;
    item.id = itemVar["id"] | "";
    item.title = itemVar["title"] | "";
    item.subtitle = itemVar["subtitle"] | "";
    item.quantity = itemVar["quantity"] | 0;
    item.unitPrice = itemVar["unitPrice"] | 0.0f;
    item.lineTotal = itemVar["lineTotal"] | 0.0f;
    basket.items.push_back(item);
  }
  return true;
}

bool readRightPanelFromJson(const JsonVariantConst& src, RightPanelState& panel) {
  if (!src.is<JsonObjectConst>()) {
    return false;
  }

  panel.screenId = src["screenId"] | panel.screenId;
  panel.mode = src["mode"] | "choice";
  panel.title = src["title"] | panel.title;
  panel.subtitle = src["subtitle"] | "";
  panel.image.type = src["image"]["type"] | "placeholder";
  panel.image.label = src["image"]["label"] | "Pizza image";
  panel.image.path = src["image"]["path"] | "";

  panel.buttons.clear();
  JsonArrayConst buttons = src["buttons"].as<JsonArrayConst>();
  for (JsonVariantConst btnVar : buttons) {
    if ((int)panel.buttons.size() >= MAX_VISIBLE_BUTTONS) {
      break;
    }
    ButtonDef btn;
    btn.id = btnVar["id"] | "";
    btn.label = btnVar["label"] | "";
    btn.subtitle = btnVar["subtitle"] | "";
    btn.style = btnVar["style"] | "primary";
    panel.buttons.push_back(btn);
  }
  return true;
}

void writeBasketJson(JsonObject obj, const BasketState& basket) {
  obj["title"] = basket.title;
  obj["currency"] = basket.currency;
  obj["status"] = basket.status;
  JsonArray items = obj["items"].to<JsonArray>();
  for (const BasketItem& item : basket.items) {
    JsonObject out = items.add<JsonObject>();
    out["id"] = item.id;
    out["title"] = item.title;
    out["subtitle"] = item.subtitle;
    out["quantity"] = item.quantity;
    out["unitPrice"] = item.unitPrice;
    out["lineTotal"] = item.lineTotal;
  }
  obj["subtotal"] = basket.subtotal;
  obj["discount"] = basket.discount;
  obj["delivery"] = basket.delivery;
  obj["total"] = basket.total;
}

void writeRightPanelJson(JsonObject obj, const RightPanelState& panel) {
  obj["screenId"] = panel.screenId;
  obj["mode"] = panel.mode;
  obj["title"] = panel.title;
  obj["subtitle"] = panel.subtitle;

  JsonObject image = obj["image"].to<JsonObject>();
  image["type"] = panel.image.type;
  image["label"] = panel.image.label;
  if (!panel.image.path.isEmpty()) {
    image["path"] = panel.image.path;
  }

  JsonArray buttons = obj["buttons"].to<JsonArray>();
  for (const ButtonDef& button : panel.buttons) {
    JsonObject btn = buttons.add<JsonObject>();
    btn["id"] = button.id;
    btn["label"] = button.label;
    if (!button.subtitle.isEmpty()) {
      btn["subtitle"] = button.subtitle;
    }
    if (!button.style.isEmpty()) {
      btn["style"] = button.style;
    }
  }
}
}  // namespace

ApiServer::ApiServer() : server_(80) {}

void ApiServer::begin() {
  const char* headerKeys[] = {"X-Api-Token"};
  server_.collectHeaders(headerKeys, 1);

  server_.on("/api/health", HTTP_GET, [this]() { handleHealth(); });
  server_.on("/api/state", HTTP_GET, [this]() { handleState(); });
  server_.on("/api/events/latest", HTTP_GET, [this]() { handleLatestEvent(); });
  server_.on("/api/config", HTTP_POST, [this]() { handleConfig(); });
  server_.on("/api/basket", HTTP_POST, [this]() { handleBasket(); });
  server_.on("/api/right-panel", HTTP_POST, [this]() { handleRightPanel(); });
  server_.on("/api/customisation", HTTP_POST, [this]() { handleCustomisation(); });
  server_.on("/api/screen", HTTP_POST, [this]() { handleScreen(); });
  server_.on("/api/refresh", HTTP_POST, [this]() { handleRefresh(); });
  server_.on("/api/clear", HTTP_POST, [this]() { handleClear(); });

  server_.begin();
}

void ApiServer::handleClient() {
  server_.handleClient();
}

bool ApiServer::validateToken() {
  const String token = server_.header("X-Api-Token");
  return token == g_deviceConfig.apiToken;
}

bool ApiServer::parseJsonBody(JsonDocument& doc) {
  const String body = server_.arg("plain");
  if (body.isEmpty() || body.length() > 16384) {
    return false;
  }
  return deserializeJson(doc, body) == DeserializationError::Ok;
}

void ApiServer::sendJson(int statusCode, const JsonDocument& doc) {
  String payload;
  serializeJson(doc, payload);
  server_.send(statusCode, "application/json", payload);
}

void ApiServer::handleHealth() {
  JsonDocument doc;
  doc["deviceId"] = g_deviceConfig.deviceId;
  doc["status"] = "ok";
  doc["ip"] = WiFi.localIP().toString();
  doc["basketItems"] = (int)g_basketState.items.size();
  doc["screenId"] = g_rightPanelState.screenId;
  doc["uptimeMs"] = millis();
  sendJson(200, doc);
}

void ApiServer::handleState() {
  JsonDocument doc;
  doc["deviceId"] = g_deviceConfig.deviceId;
  writeBasketJson(doc["basket"].to<JsonObject>(), g_basketState);
  writeRightPanelJson(doc["rightPanel"].to<JsonObject>(), g_rightPanelState);
  sendJson(200, doc);
}

void ApiServer::handleLatestEvent() {
  JsonDocument doc;
  doc["deviceId"] = g_deviceConfig.deviceId;
  doc["event"] = g_latestTouchEvent.event;
  if (g_latestTouchEvent.event != "none") {
    doc["screenId"] = g_latestTouchEvent.screenId;
    doc["buttonId"] = g_latestTouchEvent.buttonId;
    doc["label"] = g_latestTouchEvent.label;
    doc["timestampMs"] = g_latestTouchEvent.timestampMs;
  }
  sendJson(200, doc);
}

void ApiServer::handleConfig() {
  if (!validateToken()) {
    JsonDocument denied;
    denied["error"] = "unauthorized";
    sendJson(401, denied);
    return;
  }

  JsonDocument doc;
  if (!parseJsonBody(doc)) {
    JsonDocument bad;
    bad["error"] = "invalid_json";
    sendJson(400, bad);
    return;
  }

  if (doc["deviceId"].is<const char*>()) {
    g_deviceConfig.deviceId = doc["deviceId"].as<String>();
  }
  if (doc["callbackUrl"].is<const char*>()) {
    g_deviceConfig.callbackUrl = doc["callbackUrl"].as<String>();
  }
  if (doc["apiToken"].is<const char*>()) {
    g_deviceConfig.apiToken = doc["apiToken"].as<String>();
  }

  JsonDocument out;
  out["ok"] = true;
  sendJson(200, out);
}

void ApiServer::handleBasket() {
  if (!validateToken()) {
    JsonDocument denied;
    denied["error"] = "unauthorized";
    sendJson(401, denied);
    return;
  }

  JsonDocument doc;
  if (!parseJsonBody(doc) || !readBasketFromJson(doc.as<JsonObjectConst>(), g_basketState)) {
    JsonDocument bad;
    bad["error"] = "invalid_json";
    sendJson(400, bad);
    return;
  }

  Renderer::renderAll();
  JsonDocument out;
  out["ok"] = true;
  out["rendered"] = true;
  sendJson(200, out);
}

void ApiServer::handleRightPanel() {
  if (!validateToken()) {
    JsonDocument denied;
    denied["error"] = "unauthorized";
    sendJson(401, denied);
    return;
  }

  JsonDocument doc;
  if (!parseJsonBody(doc) || !readRightPanelFromJson(doc.as<JsonObjectConst>(), g_rightPanelState)) {
    JsonDocument bad;
    bad["error"] = "invalid_json";
    sendJson(400, bad);
    return;
  }

  Renderer::renderAll();
  JsonDocument out;
  out["ok"] = true;
  out["rendered"] = true;
  sendJson(200, out);
}

void ApiServer::handleCustomisation() {
  if (!validateToken()) {
    JsonDocument denied;
    denied["error"] = "unauthorized";
    sendJson(401, denied);
    return;
  }

  JsonDocument doc;
  if (!parseJsonBody(doc)) {
    JsonDocument bad;
    bad["error"] = "invalid_json";
    sendJson(400, bad);
    return;
  }

  RightPanelState panel;
  panel.screenId = doc["screenId"] | "customisation";
  panel.mode = "choice";
  panel.title = doc["question"] | "Choose an option";
  panel.subtitle = doc["subtitle"] | "";
  panel.image.type = doc["image"]["type"] | "placeholder";
  panel.image.label = doc["image"]["label"] | "Pizza image";

  JsonArrayConst options = doc["options"].as<JsonArrayConst>();
  for (JsonVariantConst opt : options) {
    if ((int)panel.buttons.size() >= MAX_VISIBLE_BUTTONS) {
      break;
    }
    ButtonDef button;
    button.id = opt["id"] | "";
    button.label = opt["label"] | "";
    button.subtitle = opt["subtitle"] | "";
    button.style = opt["style"] | "primary";
    panel.buttons.push_back(button);
  }

  g_rightPanelState = panel;
  Renderer::renderAll();

  JsonDocument out;
  out["ok"] = true;
  out["rendered"] = true;
  sendJson(200, out);
}

void ApiServer::handleScreen() {
  if (!validateToken()) {
    JsonDocument denied;
    denied["error"] = "unauthorized";
    sendJson(401, denied);
    return;
  }

  JsonDocument doc;
  if (!parseJsonBody(doc)) {
    JsonDocument bad;
    bad["error"] = "invalid_json";
    sendJson(400, bad);
    return;
  }

  if (!readBasketFromJson(doc["basket"], g_basketState) || !readRightPanelFromJson(doc["rightPanel"], g_rightPanelState)) {
    JsonDocument bad;
    bad["error"] = "invalid_payload";
    sendJson(400, bad);
    return;
  }

  Renderer::renderAll();

  JsonDocument out;
  out["ok"] = true;
  out["rendered"] = true;
  sendJson(200, out);
}

void ApiServer::handleRefresh() {
  if (!validateToken()) {
    JsonDocument denied;
    denied["error"] = "unauthorized";
    sendJson(401, denied);
    return;
  }

  Renderer::forceRefresh();
  JsonDocument out;
  out["ok"] = true;
  out["rendered"] = true;
  sendJson(200, out);
}

void ApiServer::handleClear() {
  if (!validateToken()) {
    JsonDocument denied;
    denied["error"] = "unauthorized";
    sendJson(401, denied);
    return;
  }

  resetToWelcomeState();
  Renderer::renderAll();

  JsonDocument out;
  out["ok"] = true;
  out["rendered"] = true;
  sendJson(200, out);
}
