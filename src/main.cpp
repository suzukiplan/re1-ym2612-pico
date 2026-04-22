#include "ArduinoPlatform.hpp"

#include "VGMPlayer.hpp"
#include "YM2612.hpp"
#include "YM2612Bus.hpp"
#include "rom_song.hpp"

namespace {

re1::YM2612Bus g_bus;
re1::YM2612 g_ym2612(g_bus);
re1::VGMPlayer g_vgmPlayer(g_ym2612, rom_song, rom_song_size);

void logPinMapping() {
  Serial.println("Pin mapping:");
  Serial.println("  D0-D7 -> GPIO2-GPIO9");
  Serial.println("  A0    -> GPIO10");
  Serial.println("  A1    -> GPIO11");
  Serial.println("  /WR   -> GPIO18");
  Serial.println("  /CS   -> GPIO19");
  Serial.println("  /IC   -> GPIO20");
}

void logVgmInfo() {
  Serial.print("rom_song bytes: ");
  Serial.println(static_cast<unsigned long>(rom_song_size));
  Serial.print("VGM version: 0x");
  Serial.println(static_cast<unsigned long>(g_vgmPlayer.version()), HEX);
  Serial.print("Data offset: 0x");
  Serial.println(static_cast<unsigned long>(g_vgmPlayer.dataStartOffset()), HEX);
  Serial.print("Loop offset: 0x");
  Serial.println(static_cast<unsigned long>(g_vgmPlayer.loopOffset()), HEX);
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

  if (!g_vgmPlayer.begin()) {
    Serial.println("Failed to parse rom_song VGM header.");
    return;
  }

  logVgmInfo();
  Serial.println("VGM playback started.");
}

void loop() {
  if (!g_vgmPlayer.isReady()) {
    static bool logged = false;
    if (!logged) {
      Serial.print("Playback stopped. Unknown/invalid VGM command: 0x");
      Serial.println(static_cast<unsigned long>(g_vgmPlayer.lastUnknownCommand()), HEX);
      logged = true;
    }
    return;
  }

  g_vgmPlayer.tick();
}
