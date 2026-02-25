#pragma once
#include <Arduino.h>
#include "config.h"

// AT helpers
bool sendAT(const String &cmd, const String &expect, uint32_t timeout = AT_CMD_TIMEOUT);
String sendATGet(const String &cmd, uint32_t timeout = AT_CMD_TIMEOUT);

// Modem power + LTE
void powerOnModem();
bool waitForModem();
bool setupLTE();

// GNSS + LBS
bool startGNSS();
bool getLocation(float &latitude, float &longitude, float &speedKmh,
                 String &datetime, String &source);
