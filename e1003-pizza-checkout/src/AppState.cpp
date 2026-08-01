#include "AppState.h"

BasketState g_basketState;
RightPanelState g_rightPanelState;
DeviceConfig g_deviceConfig;
TouchEvent g_latestTouchEvent;
std::vector<ButtonRect> g_buttonRects;

void resetToWelcomeState() {
  g_basketState = BasketState();
  g_rightPanelState = RightPanelState();
  g_rightPanelState.image.type = "placeholder";
  g_rightPanelState.image.label = "Pizza bot";
  g_rightPanelState.buttons.clear();

  ButtonDef startButton;
  startButton.id = "start-order";
  startButton.label = "Start order";
  startButton.style = "primary";
  g_rightPanelState.buttons.push_back(startButton);

  g_latestTouchEvent = TouchEvent();
  g_buttonRects.clear();
}
