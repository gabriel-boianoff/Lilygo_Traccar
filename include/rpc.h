#pragma once
#include <Arduino.h>
#include "config.h"
#include "mqtt.h"
#include "modem.h"
#include "battery.h"

// Initialize RPC (subscribe topics)
void rpcInit();

// Poll modem for MQTT RPC messages and handle them
void rpcLoop();
