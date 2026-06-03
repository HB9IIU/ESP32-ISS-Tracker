#pragma once

#ifdef HAS_CAPTIVE_PORTAL

#include <TFT_eSPI.h>

bool hasValidConfig();
void loadConfig();
void startProvisioner(TFT_eSPI &tft);

#endif // HAS_CAPTIVE_PORTAL
