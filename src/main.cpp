#include <Arduino.h>
#include "config.h"
#include "modem.h"
#include "battery.h"
#include "telemetry.h"

// Define our tracker states
enum TrackerState {
  STATE_INIT_HARDWARE,
  STATE_WAIT_MODEM,
  STATE_SETUP_NETWORK,
  STATE_START_GNSS,
  STATE_READY,
  STATE_ERROR
};

TrackerState currentState = STATE_INIT_HARDWARE;
unsigned long lastLocationPublish = 0;
unsigned long errorTimer = 0;

void setup() {
  SerialMon.begin(115200);
  delay(2000);

  SerialMon.println();
  SerialMon.println("=== T-SIM7670G-S3 -> Traccar UDP Tracker (FSM) ===");
  
  // We leave the rest for the state machine to handle
}

void loop() {
  switch (currentState) {
    case STATE_INIT_HARDWARE:
      SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);

      pinMode(BAT_EN_PIN, OUTPUT);
      digitalWrite(BAT_EN_PIN, LOW);
      pinMode(BAT_ADC_PIN, INPUT);

      pinMode(BOARD_LED_PIN, OUTPUT);
      digitalWrite(BOARD_LED_PIN, HIGH);   // LED OFF for active-low

      pinMode(RELAY_PIN, OUTPUT);
      digitalWrite(RELAY_PIN, LOW);        // relay off

      powerOnModem();
      
      currentState = STATE_WAIT_MODEM;
      break;

    case STATE_WAIT_MODEM:
      if (waitForModem()) {
        currentState = STATE_SETUP_NETWORK;
      } else {
        SerialMon.println("[FATAL] Modem not responding.");
        errorTimer = millis();
        currentState = STATE_ERROR;
      }
      break;

    case STATE_SETUP_NETWORK:
      if (setupLTE()) {
        currentState = STATE_START_GNSS;
      } else {
        SerialMon.println("[FATAL] LTE setup failed.");
        errorTimer = millis();
        currentState = STATE_ERROR;
      }
      break;

    case STATE_START_GNSS:
      startGNSS();  // if GPS fails, LBS will still work
      
      lastLocationPublish = millis();
      SerialMon.println("[INFO] Initialization complete. Entering READY state.");
      currentState = STATE_READY;
      break;

    case STATE_READY:
      // Normal operating loop
      handleTelemetryLoop(lastLocationPublish);
      
      // Future upgrade: If UDP fails X times in a row, we could transition 
      // back to STATE_SETUP_NETWORK to re-establish the connection.
      break;

    case STATE_ERROR:
      // Non-blocking wait for 10 seconds before rebooting
      if (millis() - errorTimer > 10000) {
        SerialMon.println("[RECOVERY] Rebooting ESP to recover from error state...");
        ESP.restart(); // Hard reset the microcontroller
      }
      break;
  }

  delay(50);
}