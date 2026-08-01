#pragma once

#include <Arduino.h>
#include <vector>

constexpr int SCREEN_W = 1872;
constexpr int SCREEN_H = 1404;
constexpr int LEFT_W = 936;
constexpr int RIGHT_X = 936;
constexpr int MAX_VISIBLE_ITEMS = 5;
constexpr int MAX_VISIBLE_BUTTONS = 6;

struct BasketItem {
  String id;
  String title;
  String subtitle;
  int quantity = 0;
  float unitPrice = 0.0f;
  float lineTotal = 0.0f;
};

struct BasketState {
  String title = "Your basket";
  String currency = "GBP";
  String status;
  std::vector<BasketItem> items;
  float subtotal = 0.0f;
  float discount = 0.0f;
  float delivery = 0.0f;
  float total = 0.0f;
};

struct ImageSpec {
  String type = "placeholder";
  String label = "Pizza image";
  String path;
};

struct ButtonDef {
  String id;
  String label;
  String subtitle;
  String style = "primary";
};

struct RightPanelState {
  String screenId = "welcome";
  String mode = "choice";
  String title = "Welcome to Contoso Pizza";
  String subtitle = "Speak to Reachy or tap below";
  ImageSpec image;
  std::vector<ButtonDef> buttons;
};

struct ButtonRect {
  String id;
  String label;
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
};

struct TouchEvent {
  String event = "none";
  String screenId;
  String buttonId;
  String label;
  uint32_t timestampMs = 0;
};

struct DeviceConfig {
  String deviceId = "e1003-pizza-01";
  String callbackUrl;
  String apiToken = "dev-secret";
};
