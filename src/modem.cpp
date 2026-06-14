#include "modem.h"

// =====================================================
// AT helpers
// =====================================================

bool sendAT(const String &cmd, const String &expect, uint32_t timeout) {
  while (SerialAT.available()) SerialAT.read();  // flush

  SerialMon.print(">> ");
  SerialMon.println(cmd);

  if (cmd.length() > 0) {
    SerialAT.print(cmd);
    SerialAT.print("\r");
  }

  String resp;
  unsigned long start = millis();

  while (millis() - start < timeout) {
    while (SerialAT.available()) {
      char c = (char)SerialAT.read();
      resp += c;
    }

    if (!expect.isEmpty() && resp.indexOf(expect) != -1) {
      SerialMon.print("<< ");
      SerialMon.println(resp);
      return true;
    }

    if (resp.indexOf("ERROR") != -1) {
      SerialMon.print("<< ");
      SerialMon.println(resp);
      return false;
    }
  }

  SerialMon.print("<< (timeout) ");
  SerialMon.println(resp);
  return false;
}

String sendATGet(const String &cmd, uint32_t timeout) {
  while (SerialAT.available()) SerialAT.read();

  SerialMon.print(">> ");
  SerialMon.println(cmd);

  if (cmd.length() > 0) {
    SerialAT.print(cmd);
    SerialAT.print("\r");
  }

  String resp;
  unsigned long start = millis();

  while (millis() - start < timeout) {
    while (SerialAT.available()) {
      char c = (char)SerialAT.read();
      resp += c;
    }
  }

  SerialMon.print("<< ");
  SerialMon.println(resp);
  return resp;
}

// =====================================================
// Modem power + LTE
// =====================================================

void powerOnModem() {
  SerialMon.println("[INFO] Powering modem...");

  pinMode(MODEM_DTR_PIN, OUTPUT);
  digitalWrite(MODEM_DTR_PIN, LOW);  // keep awake

  pinMode(MODEM_RESET_PIN, OUTPUT);
  digitalWrite(MODEM_RESET_PIN, HIGH);
  delay(100);
  digitalWrite(MODEM_RESET_PIN, LOW);
  delay(2600);
  digitalWrite(MODEM_RESET_PIN, HIGH);

  pinMode(MODEM_PWRKEY_PIN, OUTPUT);
  digitalWrite(MODEM_PWRKEY_PIN, LOW);
  delay(100);
  digitalWrite(MODEM_PWRKEY_PIN, HIGH);
  delay(1200);
  digitalWrite(MODEM_PWRKEY_PIN, LOW);

  SerialMon.println("[INFO] Modem power sequence done.");
}

static bool waitForNetworkRegistration() {
  SerialMon.println("[INFO] Waiting for network registration...");
  for (int i = 0; i < 60; i++) {
    String resp = sendATGet("AT+CGREG?", 1000);
    if (resp.indexOf(",1") != -1 || resp.indexOf(",5") != -1) {
      SerialMon.println("[INFO] Registered on network.");
      return true;
    }
    delay(1000);
  }
  SerialMon.println("[ERROR] Network registration failed.");
  return false;
}

bool waitForModem() {
  SerialMon.println("[INFO] Waiting for modem (AT)...");
  for (int i = 0; i < 20; i++) {
    if (sendAT("AT", "OK", 500)) {
      SerialMon.println("[INFO] Modem responded to AT.");
      String imei = sendATGet("AT+CGSN", 1500);
      imei.replace("OK", ""); // Clean up the modem's 'OK' response
      imei.trim();            // Remove invisible newlines
      SerialMon.println("[INFO] Modem initialized. IMEI: " + imei);
      return true;
    }
    delay(500);
  }
  SerialMon.println("[ERROR] Modem did NOT respond to AT.");
  return false;
}

bool setupLTE() {
  SerialMon.println("[INFO] Configuring LTE...");

  if (!sendAT("ATE0", "OK", 2000)) return false;
  if (!sendAT("AT+CMEE=2", "OK", 2000)) return false;
  if (!sendAT("AT+CPIN?", "READY", 5000)) return false;

  String apnCmd = String("AT+CGDCONT=1,\"IP\",\"") + APN + "\"";
  if (!sendAT(apnCmd, "OK", 5000)) return false;

  if (!waitForNetworkRegistration()) return false;

  if (!sendAT("AT+CGATT=1", "OK", 15000)) return false;
  if (!sendAT("AT+CGACT=1,1", "OK", 15000)) return false;

  String r = sendATGet("AT+NETOPEN", 8000);
  if (r.indexOf("OK") < 0 && r.indexOf("+NETOPEN: 0") < 0 && r.indexOf("already") < 0) {
    r = sendATGet("", 8000);
    if (r.indexOf("+NETOPEN: 0") < 0 && r.indexOf("already") < 0) {
      SerialMon.println("[ERROR] NETOPEN failed.");
      return false;
    }
  }

  r = sendATGet("AT+IPADDR", 3000);
  if (r.indexOf("+IPADDR:") < 0) {
    SerialMon.println("[WARN] No IPADDR line, but continuing.");
  }

  SerialMon.println("[INFO] LTE data should be up now.");
  return true;
}

// =====================================================
// GNSS + LBS
// =====================================================

bool startGNSS() {
  SerialMon.println("[INFO] Enabling GNSS fast-fix...");

  if (!sendAT("AT+CGNSSPWR=1", "OK", 5000)) {
    SerialMon.println("[ERROR] CGNSSPWR failed.");
    return false;
  }

  if (!sendAT("AT+CNETSTART", "OK", 10000)) {
    SerialMon.println("[WARN] CNETSTART failed or not supported.");
  }

  sendAT("AT+CGDRT=4,1", "OK", 3000);
  sendAT("AT+CGSETV=4,1", "OK", 3000);

  sendAT("AT+CGPSCOLD", "OK", 5000);
  sendAT("AT+CGNSSMODE=3", "OK", 5000);

  SerialMon.println("[INFO] GNSS init complete. First fix may take some seconds.");
  return true;
}

static bool convertToDecimalDegrees(const String &nmea, float &result) {
  if (nmea.length() < 3) return false;
  int dotIndex = nmea.indexOf('.');
  if (dotIndex < 0) return false;

  int degDigits = (dotIndex == 4) ? 2 : 3;
  String degStr = nmea.substring(0, degDigits);
  String minStr = nmea.substring(degDigits);

  float deg = degStr.toFloat();
  float minutes = minStr.toFloat();

  result = deg + (minutes / 60.0f);
  return true;
}

static String buildGpsDateTime(const String &dateStr, const String &timeStr) {
  // Parses NMEA GPS time (DDMMYY and HHMMSS) into standard YYYY-MM-DDTHH:MM:SS
  if (dateStr.length() < 6 || timeStr.length() < 6) return "";

  String dd = dateStr.substring(0, 2);
  String mm = dateStr.substring(2, 4);
  String yy = "20" + dateStr.substring(4, 6);

  String hh  = timeStr.substring(0, 2);
  String min = timeStr.substring(2, 4);
  String ss  = timeStr.substring(4, 6);

  return yy + "-" + mm + "-" + dd + "T" + hh + ":" + min + ":" + ss;
}

static String getNetworkDateTime() {
  // Parses Cellular Network time "yy/MM/dd,hh:mm:ss±zz" into YYYY-MM-DDTHH:MM:SS
  String resp = sendATGet("AT+CCLK?", 3000);
  int firstQuote = resp.indexOf('"');
  int secondQuote = resp.indexOf('"', firstQuote + 1);
  
  if (firstQuote >= 0 && secondQuote > firstQuote) {
    String raw = resp.substring(firstQuote + 1, secondQuote);
    if (raw.length() >= 17) {
      String yy = "20" + raw.substring(0, 2);
      String mm = raw.substring(3, 5);
      String dd = raw.substring(6, 8);
      String hh = raw.substring(9, 11);
      String min = raw.substring(12, 14);
      String ss = raw.substring(15, 17);
      
      return yy + "-" + mm + "-" + dd + "T" + hh + ":" + min + ":" + ss;
    }
  }
  return "2000-01-01T00:00:00"; // Fallback if network time fails
}

static bool getGps(float &latitude, float &longitude, float &speedKmh, String &datetime) {
  String resp = sendATGet("AT+CGPSINFO", 3000);
  int idx = resp.indexOf("+CGPSINFO:");
  if (idx == -1) return false;

  String line = resp.substring(idx + 10);
  line.trim();

  if (line.startsWith(",")) return false; // No fix yet

  String parts[12];
  int partIndex = 0;
  int start = 0;
  for (int i = 0; i < (int)line.length() && partIndex < 12; i++) {
    if (line[i] == ',') {
      parts[partIndex++] = line.substring(start, i);
      start = i + 1;
    }
  }
  if (partIndex < 12 && start < (int)line.length()) parts[partIndex++] = line.substring(start);
  if (partIndex < 8) return false;

  String latStr   = parts[0];
  String latNS    = parts[1];
  String lonStr   = parts[2];
  String lonEW    = parts[3];
  String dateStr  = parts[4];
  String timeStr  = parts[5];
  String speedStr = parts[7];

  float flat, flon;
  // (Assuming your convertToDecimalDegrees function is still right above this)
  if (!convertToDecimalDegrees(latStr, flat)) return false;
  if (!convertToDecimalDegrees(lonStr, flon)) return false;

  if (latNS == "S") flat = -flat;
  if (lonEW == "W") flon = -flon;

  latitude  = flat;
  longitude = flon;
  speedKmh = speedStr.toFloat() * 1.852f; // Knots to km/h
  datetime = buildGpsDateTime(dateStr, timeStr); // Pure UTC

  return true;
}

static bool getLbs(float &latitude, float &longitude, float &speedKmh, String &datetime) {
  String resp = sendATGet("AT+CLBS=1,1", 15000);
  int idx = resp.indexOf("+CLBS:");
  if (idx == -1) return false;

  String line = resp.substring(idx + 7);
  line.trim();

  String parts[4];
  int partIndex = 0;
  int start = 0;

  for (int i = 0; i < (int)line.length() && partIndex < 4; i++) {
    char c = line[i];
    if (c == ',' || c == '\r' || c == '\n') {
      if (i > start) parts[partIndex++] = line.substring(start, i);
      start = i + 1;
    }
  }
  if (partIndex < 4 && start < (int)line.length()) parts[partIndex++] = line.substring(start);
  if (partIndex < 4 || parts[0].toInt() != 0) return false;

  latitude  = parts[1].toFloat();
  longitude = parts[2].toFloat();
  speedKmh  = 0.0f;
  datetime  = getNetworkDateTime(); // Pure UTC from Cell Tower

  return true;
}

bool getLocation(float &latitude, float &longitude, float &speedKmh, String &datetime, String &source) {
  if (getGps(latitude, longitude, speedKmh, datetime)) {
    source = "gps";
    return true;
  }
  if (getLbs(latitude, longitude, speedKmh, datetime)) {
    source = "lbs";
    return true;
  }
  return false;
}

// =====================================================
// UDP Sockets (Traccar)
// =====================================================

bool sendUdpData(const String &server, int port, const String &payload) {
  SerialMon.println("[UDP] Opening local socket...");
  
  // SIMCOM UDP Quirk: We DO NOT put the AWS server IP here. 
  // We just open an arbitrary local UDP port (e.g., 5013).
  String openCmd = "AT+CIPOPEN=0,\"UDP\",,,5013";
  String r = sendATGet(openCmd, 8000);
  
  if (r.indexOf("+CIPOPEN: 0,0") < 0 && r.indexOf("already") < 0 && r.indexOf("OK") < 0) {
    SerialMon.println("[UDP] Open failed.");
    return false;
  }

  SerialMon.println("[UDP] Sending payload...");
  
  // SIMCOM UDP Quirk: The AWS server IP and Port must be passed HERE in the CIPSEND command!
  String sendCmd = "AT+CIPSEND=0," + String(payload.length()) + ",\"" + server + "\"," + String(port);
  SerialAT.println(sendCmd);
  
  unsigned long start = millis();
  bool gotPrompt = false;
  r = "";
  
  // Wait for the '>' prompt before sending actual data
  while(millis() - start < 3000) {
    while(SerialAT.available()) {
      r += (char)SerialAT.read();
    }
    if(r.indexOf('>') >= 0) { 
      gotPrompt = true; 
      break; 
    }
    if(r.indexOf("ERROR") >= 0) {
      break;
    }
  }

  if (!gotPrompt) {
    SerialMon.println("[UDP] No '>' prompt received.");
    sendATGet("AT+CIPCLOSE=0", 2000); // Clean up
    return false;
  }

  // Send the actual H02 string
  SerialAT.print(payload);
  
  start = millis();
  bool sendOk = false;
  r = "";
  
  // Wait for confirmation that the modem sent the bytes over the network
  while(millis() - start < 5000) {
    while(SerialAT.available()) {
      r += (char)SerialAT.read();
    }
    if(r.indexOf("OK") >= 0 || r.indexOf("+CIPSEND: 0,") >= 0) { 
      sendOk = true; 
      break; 
    }
    if(r.indexOf("ERROR") >= 0) {
      break;
    }
  }

  if (sendOk) {
    SerialMon.println("[UDP] Send OK.");
  } else {
    SerialMon.println("[UDP] Send failed.");
  }

  // Immediately close the socket to save data
  SerialMon.println("[UDP] Closing socket...");
  sendATGet("AT+CIPCLOSE=0", 3000);
  
  return sendOk;
}