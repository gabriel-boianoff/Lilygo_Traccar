#pragma once
#include <Arduino.h>

// Serial aliases
#define SerialMon Serial
#define SerialAT  Serial1

// ---------- LilyGO T-SIM7670G-S3 pins ----------
constexpr int MODEM_TX_PIN     = 11;  // ESP32 -> modem RX
constexpr int MODEM_RX_PIN     = 10;  // ESP32 <- modem TX
constexpr int MODEM_DTR_PIN    = 9;
constexpr int MODEM_RING_PIN   = 3;
constexpr int MODEM_RESET_PIN  = 17;
constexpr int MODEM_PWRKEY_PIN = 18;

// Battery measurement hardware
constexpr int BAT_ADC_PIN = 4;    // ADC
constexpr int BAT_EN_PIN  = 12;   // enable for divider

// “Heartbeat” LED (external LED recommended on this pin)
constexpr int BOARD_LED_PIN = 2;

// Relay we’ll control via RPC
constexpr int RELAY_PIN = 5;

// ---------- Network / ThingsBoard ----------
constexpr char APN[]        = "iot.1nce.net";            // 1NCE APN
constexpr char TB_HOST[]    = "mqtt.thingsboard.cloud";
constexpr int  TB_PORT      = 1883;

// Device token
constexpr char TB_TOKEN[]      = "qHpyL8Ts3XjeDj0MzIiE";
constexpr char TB_CLIENT_ID[]  = "lilygo-tsim7670g-s3-gps01";
constexpr char TB_TOPIC[]      = "v1/devices/me/telemetry";

// Timing
constexpr uint32_t AT_CMD_TIMEOUT               = 10000;
constexpr uint32_t DEFAULT_PUBLISH_INTERVAL_MS = 20000;   // 20s
