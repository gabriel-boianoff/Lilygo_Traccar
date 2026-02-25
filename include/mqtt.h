#pragma once
#include <Arduino.h>
#include "config.h"
#include "modem.h"

bool mqttConnectThingsBoard();
bool mqttPublish(const String &topic, const String &payload);

// Used by RPC module to subscribe to RPC topic
bool mqttSubscribeRpc();
