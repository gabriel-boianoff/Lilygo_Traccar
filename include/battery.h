#pragma once
#include <Arduino.h>
#include "config.h"

// Read battery via ADC (BAT_ADC_PIN + BAT_EN_PIN)
bool getBatteryFromAdc(int &levelPercent, int &millivolts, String &statusText);
