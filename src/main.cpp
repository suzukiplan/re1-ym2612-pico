#include <Arduino.h>

#include "YM2612.hpp"
#include "YM2612Bus.hpp"

namespace {

re1::YM2612Bus g_bus;
re1::YM2612 g_ym2612(g_bus);

void writeStartupTestRegisters() {
  Serial.println("Writing safe YM2612 test registers...");

  g_ym2612.writeRegister(0, 0x22, 0x00);
  Serial.println("  port 0, reg 0x22 <- 0x00 (LFO disabled)");

  g_ym2612.writeRegister(0, 0x27, 0x30);
  Serial.println("  port 0, reg 0x27 <- 0x30 (timers reset)");

  g_ym2612.writeRegister(0, 0x2B, 0x00);
  Serial.println("  port 0, reg 0x2B <- 0x00 (DAC disabled)");

  g_ym2612.writeRegister(1, 0xB6, 0xC0);
  Serial.println("  port 1, reg 0xB6 <- 0xC0 (safe pan/AMS/PMS default)");
}

void logPinMapping() {
  Serial.println("Pin mapping:");
  Serial.println("  D0-D7 -> GPIO2-GPIO9");
  Serial.println("  A0    -> GPIO10");
  Serial.println("  A1    -> GPIO11");
  Serial.println("  /WR   -> GPIO18");
  Serial.println("  /CS   -> GPIO19");
  Serial.println("  /IC   -> GPIO20");
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("RE1-YM2612 minimal bus bring-up");
  logPinMapping();

  g_ym2612.begin();
  Serial.println("YM2612 reset complete.");

  g_ym2612.initializeSafeDefaults();
  Serial.println("YM2612 safe defaults applied.");

  writeStartupTestRegisters();
  Serial.println("Initialization finished.");
}

void loop() {
}
