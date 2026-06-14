#pragma once
#include <Arduino.h>
#include "config.h"
#include "modem.h"
#include "battery.h"

// Periodic telemetry handler - ONLY ONE ARGUMENT NOW
void handleTelemetryLoop(unsigned long &lastLocationPublish);

// Tracker “on/off” logical state
void setTrackerActive(bool active);
bool isTrackerActive();