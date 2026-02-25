#include "telemetry.h"

static bool trackerActive = true;  // true = normal, false = “off/sleep”

void setTrackerActive(bool active) {
  trackerActive = active;
}

bool isTrackerActive() {
  return trackerActive;
}

void handleTelemetryLoop(bool &mqttReady, unsigned long &lastLocationPublish) {
  if (!mqttReady) {
    SerialMon.println("[MQTT] Not ready, retrying connect...");
    if (!mqttConnectThingsBoard()) {
      SerialMon.println("[MQTT] Reconnect failed, retry in 10s.");
      delay(10000);
      return;
    }
    mqttReady = true;
    lastLocationPublish = millis();
  }

  // 🔴 If tracker is “off”, keep MQTT alive but do NOT send telemetry
  if (!trackerActive) {
    return;
  }

  if (millis() - lastLocationPublish >= DEFAULT_PUBLISH_INTERVAL_MS) {
    lastLocationPublish = millis();

    float latitude, longitude, speedKmh;
    String datetime;
    String source;

    int    batteryLevel  = -1;
    int    batteryMv     = 0;
    String batteryStatus = "unknown";

    getBatteryFromAdc(batteryLevel, batteryMv, batteryStatus);

    if (getLocation(latitude, longitude, speedKmh, datetime, source)) {
      String json = String("{\"latitude\":")      + String(latitude,  6) +
                    ",\"longitude\":"            + String(longitude, 6) +
                    ",\"speed\":"                + String(speedKmh,  2) +
                    ",\"datetime\":\""           + datetime + "\"" +
                    ",\"source\":\""             + source   + "\"" +
                    ",\"battery_level\":"        + String(batteryLevel) +
                    ",\"battery_status\":\""     + batteryStatus + "\"" +
                    ",\"battery_voltage_mv\":"   + String(batteryMv) +
                    "}";

      SerialMon.print("[INFO] Telemetry: ");
      SerialMon.println(json);

      if (!mqttPublish(TB_TOPIC, json)) {
        SerialMon.println("[MQTT] Publish failed, marking not ready.");
        mqttReady = false;
      } else {
        // Blink LED on success (assuming active-LOW)
        digitalWrite(BOARD_LED_PIN, LOW);   // ON
        delay(150);
        digitalWrite(BOARD_LED_PIN, HIGH);  // OFF
      }
    } else {
      SerialMon.println("[WARN] No GPS/LBS location available, skipping publish.");
    }
  }
}
