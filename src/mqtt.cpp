#include "mqtt.h"

static bool mqttStart() {
  SerialMon.println("[MQTT] Starting service...");

  String r = sendATGet("AT+CMQTTSTART", 5000);
  if (r.indexOf("+CMQTTSTART: 0") < 0 && r.indexOf("OK") < 0) {
    SerialMon.println("[MQTT] CMQTTSTART failed.");
    return false;
  }

  r = sendATGet("AT+CMQTTACCQ=0,\"esp32s3-7670\"", 2000);
  if (r.indexOf("OK") < 0) {
    SerialMon.println("[MQTT] CMQTTACCQ failed.");
    return false;
  }

  sendATGet("AT+CMQTTCFG=\"version\",0,4", 1000);
  return true;
}

bool mqttConnectThingsBoard() {
  if (!mqttStart()) return false;

  String cmd = "AT+CMQTTCONNECT=0,\"tcp://";
  cmd += TB_HOST;
  cmd += ":";
  cmd += TB_PORT;
  cmd += "\",60,1,\"";
  cmd += TB_TOKEN;
  cmd += "\"";

  String r = sendATGet(cmd, 8000);
  if (r.indexOf("+CMQTTCONNECT: 0,0") >= 0 || r.indexOf("OK") >= 0) {
    SerialMon.println("[MQTT] Connected to ThingsBoard.");
    return true;
  }

  SerialMon.println("[MQTT] Connect failed:");
  SerialMon.println(r);
  return false;
}

bool mqttPublish(const String &topic, const String &payload) {
  String r;
  unsigned long start;
  bool gotPrompt = false;
  bool gotOK = false;

  // Topic
  SerialAT.print("AT+CMQTTTOPIC=0,");
  SerialAT.println(topic.length());

  r = "";
  start = millis();
  while (millis() - start < 2000) {
    while (SerialAT.available()) {
      r += (char)SerialAT.read();
    }
    if (r.indexOf('>') >= 0) { gotPrompt = true; break; }
    if (r.indexOf("ERROR") >= 0) {
      SerialMon.println("[MQTT] ERROR for CMQTTTOPIC.");
      return false;
    }
  }
  SerialMon.println("[MQTT] TOPIC cmd resp:");
  SerialMon.println(r);
  if (!gotPrompt) {
    SerialMon.println("[MQTT] No '>' for CMQTTTOPIC.");
    return false;
  }

  SerialAT.print(topic);

  r = "";
  start = millis();
  while (millis() - start < 2000) {
    while (SerialAT.available()) {
      r += (char)SerialAT.read();
    }
    if (r.indexOf("OK") >= 0) { gotOK = true; break; }
    if (r.indexOf("ERROR") >= 0) {
      SerialMon.println("[MQTT] ERROR after topic data.");
      return false;
    }
  }
  SerialMon.println("[MQTT] TOPIC data resp:");
  SerialMon.println(r);
  if (!gotOK) {
    SerialMon.println("[MQTT] No OK after topic text.");
    return false;
  }

  // Payload
  SerialAT.print("AT+CMQTTPAYLOAD=0,");
  SerialAT.println(payload.length());

  r = "";
  start = millis();
  gotPrompt = false;
  while (millis() - start < 2000) {
    while (SerialAT.available()) {
      r += (char)SerialAT.read();
    }
    if (r.indexOf('>') >= 0) { gotPrompt = true; break; }
    if (r.indexOf("ERROR") >= 0) {
      SerialMon.println("[MQTT] ERROR for CMQTTPAYLOAD.");
      return false;
    }
  }
  SerialMon.println("[MQTT] PAYLOAD cmd resp:");
  SerialMon.println(r);
  if (!gotPrompt) {
    SerialMon.println("[MQTT] No '>' for CMQTTPAYLOAD.");
    return false;
  }

  SerialAT.print(payload);

  r = "";
  start = millis();
  gotOK = false;
  while (millis() - start < 2000) {
    while (SerialAT.available()) {
      r += (char)SerialAT.read();
    }
    if (r.indexOf("OK") >= 0) { gotOK = true; break; }
    if (r.indexOf("ERROR") >= 0) {
      SerialMon.println("[MQTT] ERROR after payload data.");
      return false;
    }
  }
  SerialMon.println("[MQTT] PAYLOAD data resp:");
  SerialMon.println(r);
  if (!gotOK) {
    SerialMon.println("[MQTT] No OK after payload.");
    return false;
  }

  // Publish
  r = sendATGet("AT+CMQTTPUB=0,1,60", 4000);
  SerialMon.println("[MQTT] PUBLISH resp:");
  SerialMon.println(r);

  if (r.indexOf("+CMQTTPUB: 0,0") >= 0 || r.indexOf("OK") >= 0) {
    SerialMon.println("[MQTT] Publish OK.");
    return true;
  }

  SerialMon.println("[MQTT] Publish failed.");
  return false;
}

// ---------- RPC subscription ----------

bool mqttSubscribeRpc() {
  String topic = "v1/devices/me/rpc/request/+";
  int len = topic.length();

  SerialMon.println("[MQTT] Subscribing RPC topic (CMQTTSUB direct)...");

  // Direct subscribe: AT+CMQTTSUB=<client>,<len>,<qos>
  String cmd = "AT+CMQTTSUB=0,";
  cmd += len;
  cmd += ",1";   // QoS 1

  // Send command and wait for '>' prompt
  SerialAT.print(cmd);
  SerialAT.print("\r");

  String r;
  unsigned long start = millis();
  bool gotPrompt = false;

  while (millis() - start < 5000) {
    while (SerialAT.available()) {
      r += (char)SerialAT.read();
    }
    if (r.indexOf('>') >= 0) {           // ready for topic text
      gotPrompt = true;
      break;
    }
    if (r.indexOf("ERROR") >= 0) {
      SerialMon.println("[MQTT] CMQTTSUB error (no prompt):");
      SerialMon.println(r);
      return false;
    }
  }

  SerialMon.println("[MQTT] SUB cmd resp:");
  SerialMon.println(r);
  if (!gotPrompt) {
    SerialMon.println("[MQTT] No '>' prompt for CMQTTSUB.");
    return false;
  }

  // Send topic string
  SerialAT.print(topic);

  // Wait for OK and +CMQTTSUB: 0,0
  r = "";
  start = millis();
  bool gotOK = false;
  bool gotResult = false;

  while (millis() - start < 5000) {
    while (SerialAT.available()) {
      r += (char)SerialAT.read();
    }
    if (r.indexOf("OK") >= 0)        gotOK = true;
    if (r.indexOf("+CMQTTSUB: 0,0") >= 0) {  // success
      gotResult = true;
      break;
    }
    if (r.indexOf("ERROR") >= 0) {
      SerialMon.println("[MQTT] CMQTTSUB data error:");
      SerialMon.println(r);
      return false;
    }
  }

  SerialMon.println("[MQTT] SUB data resp:");
  SerialMon.println(r);

  if (gotOK && gotResult) {
    SerialMon.println("[MQTT] RPC subscription OK.");
    return true;
  }

  SerialMon.println("[MQTT] RPC subscription FAILED.");
  return false;
}
