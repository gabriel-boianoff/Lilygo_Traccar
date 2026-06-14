#include "telemetry.h"
#include "config.h"
#include "modem.h"

static bool trackerActive = true;  // true = normal, false = “off/sleep”

void setTrackerActive(bool active) {
  trackerActive = active;
}

bool isTrackerActive() {
  return trackerActive;
}

void handleTelemetryLoop(unsigned long &lastLocationPublish) {
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
      
      String timeStr = "000000";
      String dateStr = "010170"; // Default epoch
      
      // We now receive standardized YYYY-MM-DDTHH:MM:SS in pure UTC from modem.cpp
      if (datetime.length() >= 19) {
        String yy = datetime.substring(2, 4);
        String mm = datetime.substring(5, 7);
        String dd = datetime.substring(8, 10);
        String hh = datetime.substring(11, 13);
        String min = datetime.substring(14, 16);
        String ss = datetime.substring(17, 19);
        
        dateStr = dd + mm + yy;    // DDMMYY
        timeStr = hh + min + ss;   // HHMMSS
      }

      String validity = (source == "gps") ? "A" : "V";

      // Convert Decimal Degrees back to DDMM.MMMM for the strict H02 parser
      char latDir = (latitude >= 0) ? 'N' : 'S';
      float absLat = abs(latitude);
      int latDeg = (int)absLat;
      float latMin = (absLat - latDeg) * 60.0f;

      char lonDir = (longitude >= 0) ? 'E' : 'W';
      float absLon = abs(longitude);
      int lonDeg = (int)absLon;
      float lonMin = (absLon - lonDeg) * 60.0f;

      // Build the properly formatted H02 UDP String
      // Notice the %06.2f for the speed parameter!
      char h02Buffer[128];
      snprintf(h02Buffer, sizeof(h02Buffer),
               "*HQ,%s,V1,%s,%s,%02d%07.4f,%c,%03d%07.4f,%c,%06.2f,000,%s,FFFFFFFF#",
               TRACCAR_DEVICE_ID,
               timeStr.c_str(),
               validity.c_str(),
               latDeg, latMin, latDir,
               lonDeg, lonMin, lonDir,
               speedKmh,
               dateStr.c_str());

      String payload = String(h02Buffer);

      SerialMon.print("[INFO] Telemetry UDP: ");
      SerialMon.println(payload);

      if (sendUdpData(TRACCAR_HOST, TRACCAR_PORT, payload)) {
        digitalWrite(BOARD_LED_PIN, LOW);   // ON
        delay(150);
        digitalWrite(BOARD_LED_PIN, HIGH);  // OFF
      } else {
        SerialMon.println("[WARN] UDP send failed.");
      }
    } else {
      SerialMon.println("[WARN] No GPS/LBS location available, skipping publish.");
    }
  }
}