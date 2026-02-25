#include <Arduino.h>
#include "config.h"
#include "modem.h"
#include "battery.h"
#include "mqtt.h"
#include "telemetry.h"
#include "rpc.h"

bool g_mqttReady = false;
unsigned long lastLocationPublish = 0;

void setup() {
  SerialMon.begin(115200);
  delay(2000);

  SerialMon.println();
  SerialMon.println("=== T-SIM7670G-S3 -> ThingsBoard Tracker (PIO) ===");

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);

  pinMode(BAT_EN_PIN, OUTPUT);
  digitalWrite(BAT_EN_PIN, LOW);

  pinMode(BAT_ADC_PIN, INPUT);

  pinMode(BOARD_LED_PIN, OUTPUT);
  digitalWrite(BOARD_LED_PIN, HIGH);   // LED OFF for active-low

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);        // relay off

  powerOnModem();

  if (!waitForModem()) {
    SerialMon.println("[FATAL] Modem not responding. Stopping.");
    return;
  }

  if (!setupLTE()) {
    SerialMon.println("[FATAL] LTE setup failed. Stopping.");
    return;
  }

  startGNSS();  // if GPS fails, LBS will still work

  if (!mqttConnectThingsBoard()) {
    SerialMon.println("[FATAL] MQTT connect failed. Stopping.");
    return;
  }

  g_mqttReady = true;
  lastLocationPublish = millis();

  // Subscribe to RPC
  rpcInit();

  SerialMon.println("[INFO] Setup complete. Telemetry every 20s.");
}

void loop() {
  // Handle RPC commands from TB (getLocation, setRelay)
  rpcLoop();

  // Handle periodic telemetry
  handleTelemetryLoop(g_mqttReady, lastLocationPublish);

  delay(50);
}
