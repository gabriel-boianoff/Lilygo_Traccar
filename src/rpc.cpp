#include "rpc.h"
#include "telemetry.h"


static bool rpcSubscribed = false;
static bool relayState = false;

// Forward
static void handleRpcMessage(const String &topic, const String &payload);

void rpcInit() {
  if (!mqttSubscribeRpc()) {
    SerialMon.println("[RPC] Subscribe failed.");
    rpcSubscribed = false;
  } else {
    rpcSubscribed = true;
  }
}

// Parse incoming URCs from modem and deliver RPC messages
void rpcLoop() {
  if (!rpcSubscribed) return;

  static String line;
  static bool inRx = false;
  static bool expectTopic = false;
  static bool expectPayload = false;
  static String curTopic;
  static String curPayload;

  while (SerialAT.available()) {
    char c = (char)SerialAT.read();
    if (c == '\r') continue;

    if (c != '\n') {
      line += c;
      continue;
    }

    // got a full line
    // SerialMon.print("[URC] "); SerialMon.println(line);

    if (line.startsWith("+CMQTTRXSTART:")) {
      inRx = true;
      expectTopic = false;
      expectPayload = false;
      curTopic = "";
      curPayload = "";
    }
    else if (line.startsWith("+CMQTTRXTOPIC:") && inRx) {
      expectTopic = true;      // next line is topic
    }
    else if (line.startsWith("+CMQTTRXPAYLOAD:") && inRx) {
      expectPayload = true;    // next line is payload
    }
    else if (line.startsWith("+CMQTTRXEND:") && inRx) {
      handleRpcMessage(curTopic, curPayload);
      inRx = false;
    }
    else {
      if (expectTopic) {
        curTopic = line;
        expectTopic = false;
      } else if (expectPayload) {
        curPayload = line;
        expectPayload = false;
      }
    }

    line = "";
  }
}

static void handleRpcMessage(const String &topic, const String &payload) {
  SerialMon.println("[RPC] Incoming message:");
  SerialMon.print("  Topic: ");   SerialMon.println(topic);
  SerialMon.print("  Payload: "); SerialMon.println(payload);

  int lastSlash = topic.lastIndexOf('/');
  String requestId = (lastSlash >= 0) ? topic.substring(lastSlash + 1) : "0";

  bool isGetLocation = payload.indexOf("\"getLocation\"") >= 0;
  bool isSetRelay    = payload.indexOf("\"setRelay\"")    >= 0;
  bool isGetRelay    = payload.indexOf("\"getRelay\"")    >= 0;
  bool isGetState    = payload.indexOf("\"getState\"")    >= 0;
  bool isSetState    = payload.indexOf("\"setState\"")    >= 0;


  if (isGetLocation) {
    float lat, lon, speedKmh;
    String dt;
    String src;

    if (getLocation(lat, lon, speedKmh, dt, src)) {
      String json = String("{\"latitude\":")      + String(lat,  6) +
                    ",\"longitude\":"            + String(lon,  6) +
                    ",\"speed\":"                + String(speedKmh, 2) +
                    ",\"datetime\":\""           + dt + "\"" +
                    ",\"source\":\""             + src + "\"" +
                    "}";

      // Publish as telemetry
      mqttPublish(TB_TOPIC, json);

      // Respond to RPC
      String respTopic = String("v1/devices/me/rpc/response/") + requestId;
      mqttPublish(respTopic, json);

      SerialMon.println("[RPC] getLocation handled.");
    } else {
      SerialMon.println("[RPC] getLocation: no fix.");
    }
  }
  else if (isSetRelay) {
  // Accept either {"state":true} or {"params":true}
  bool newState = false;
  if (payload.indexOf("\"state\"") >= 0) {
    newState = (payload.indexOf("\"state\":true") >= 0);
  } else {
    newState = (payload.indexOf("\"params\":true") >= 0);
  }

  relayState = newState;
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);

  String json = String("{\"relay_state\":") + (relayState ? "true" : "false") + "}";

  String respTopic = String("v1/devices/me/rpc/response/") + requestId;
  mqttPublish(respTopic, json);

  SerialMon.print("[RPC] setRelay -> ");
  SerialMon.println(relayState ? "ON" : "OFF");
  }
  else if (isGetRelay) {
    // NEW: return current relay state
    String json = String("{\"relay_state\":") + (relayState ? "true" : "false") + "}";

    String respTopic = String("v1/devices/me/rpc/response/") + requestId;
    mqttPublish(respTopic, json);

    SerialMon.println("[RPC] getRelay handled.");
  }
  else if (isGetState) {
    // NEW: return tracker active state
    bool active = isTrackerActive();
    String json = String("{\"state\":") + (active ? "true" : "false") + "}";

    String respTopic = String("v1/devices/me/rpc/response/") + requestId;
    mqttPublish(respTopic, json);

    SerialMon.print("[RPC] getState -> ");
    SerialMon.println(active ? "ON" : "OFF");
  }
  else if (isSetState) {
  // NEW: set tracker logical state
  // Accept either {"state":true} or {"params":true}
  bool newState = false;
  if (payload.indexOf("\"state\"") >= 0) {
    newState = (payload.indexOf("\"state\":true") >= 0);
  } else {
    newState = (payload.indexOf("\"params\":true") >= 0);
  }

  setTrackerActive(newState);

  // For safety: when turning “off”, force relay OFF too
  if (!newState) {
    relayState = false;
    digitalWrite(RELAY_PIN, LOW);
  }

  String json = String("{\"state\":") + (newState ? "true" : "false") + "}";

  String respTopic = String("v1/devices/me/rpc/response/") + requestId;
  mqttPublish(respTopic, json);

  SerialMon.print("[RPC] setState -> ");
  SerialMon.println(newState ? "ON" : "OFF");
  }
  else {
    SerialMon.println("[RPC] Unknown method.");
  }
}
