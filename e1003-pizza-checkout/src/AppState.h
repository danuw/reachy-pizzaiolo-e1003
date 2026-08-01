#pragma once

#include "Models.h"

extern BasketState g_basketState;
extern RightPanelState g_rightPanelState;
extern DeviceConfig g_deviceConfig;
extern TouchEvent g_latestTouchEvent;
extern std::vector<ButtonRect> g_buttonRects;

void resetToWelcomeState();
