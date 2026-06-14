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

// Relay (keeping the pin defined so main.cpp can safely pull it low)
constexpr int RELAY_PIN = 5;

// ---------- Network / Traccar ----------
constexpr char APN[]               = "iot.1nce.net";    // 1NCE APN

// REPLACE THIS with your actual Traccar server IP or Domain!
constexpr char TRACCAR_HOST[]      = "52.41.62.55"; 

// 5013 is the port for the lightweight H02 UDP protocol in Traccar
constexpr int  TRACCAR_PORT        = 5013;              

// This must exactly match the "Identifier" you create for the device in Traccar
constexpr char TRACCAR_DEVICE_ID[] = "864643060473974";        

// Timing
constexpr uint32_t AT_CMD_TIMEOUT              = 10000;
constexpr uint32_t DEFAULT_PUBLISH_INTERVAL_MS = 20000;   // 20s