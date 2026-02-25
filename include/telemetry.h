#pragma once
#include <Arduino.h>
#include "config.h"
#include "modem.h"
#include "battery.h"
#include "mqtt.h"

// Periodic telemetry handler
void handleTelemetryLoop(bool &mqttReady, unsigned long &lastLocationPublish);

// Tracker “on/off” logical state
void setTrackerActive(bool active);
bool isTrackerActive();