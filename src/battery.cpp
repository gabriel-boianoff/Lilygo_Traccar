#include "battery.h"

bool getBatteryFromAdc(int &levelPercent, int &millivolts, String &statusText) {
  SerialMon.println("[BAT] ADC battery read...");

  digitalWrite(BAT_EN_PIN, HIGH);
  delay(5);

  uint32_t sum = 0;
  for (int i = 0; i < 12; i++) {
    sum += analogReadMilliVolts(BAT_ADC_PIN);
    delay(2);
  }

  digitalWrite(BAT_EN_PIN, LOW);

  float adcMv = (float)sum / 12.0f;

  // 2:1 divider (approx) -> battery ~ 2x ADC
  const float DIVIDER = 2.0f;

  float battMvF = adcMv * DIVIDER;
  millivolts = (int)battMvF;
  float volts = battMvF / 1000.0f;

  // Approx mapping tuned to your pack
  const float V_MIN = 3.40f;   // empty-ish
  const float V_MAX = 3.80f;   // full (green LED)
  float pct = (volts - V_MIN) / (V_MAX - V_MIN) * 100.0f;

  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  levelPercent = (int)(pct + 0.5f);

  if (levelPercent <= 20)       statusText = "low";
  else if (levelPercent >= 80)  statusText = "full";
  else                          statusText = "normal";

  SerialMon.print("[BAT] ");
  SerialMon.print(volts, 3);
  SerialMon.print(" V  ");
  SerialMon.print(levelPercent);
  SerialMon.print("%  status=");
  SerialMon.println(statusText);

  return true;
}
