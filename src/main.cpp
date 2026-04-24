#include "ArduinoPlatform.hpp"

#include "VGMPlayer.hpp"
#include "YM2612.hpp"
#include "YM2612Bus.hpp"
#include "rom_song.hpp"

namespace
{

re1::YM2612Bus g_bus;
re1::YM2612 g_ym2612(g_bus);
re1::VGMPlayer g_vgmPlayer(g_ym2612, rom_song, rom_song_size);
unsigned long g_lastBlinkMillis = 0;
bool g_ledState = false;
volatile uint16_t g_irqTestCounter = 0;

constexpr uint8_t kUserLedPin = 25;
constexpr uint8_t kYmIrqPin = 21;
constexpr unsigned long kBlinkIntervalMs = 250;
constexpr unsigned long kIrqSelfTestTimeoutMs = 1000;
constexpr uint16_t kIrqSelfTestPassCount = 10;

void irqTestIsr()
{
    g_ym2612.setTimerA(0);
    ++g_irqTestCounter;
}

void logPinMapping()
{
    Serial.println("Pin mapping:");
    Serial.println("  D0-D7 -> GPIO2-GPIO9");
    Serial.println("  A0    -> GPIO10");
    Serial.println("  A1    -> GPIO11");
    Serial.println("  /WR   -> GPIO18");
    Serial.println("  /CS   -> GPIO19");
    Serial.println("  /IC   -> GPIO20");
    Serial.println("  /IRQ  -> GPIO21");
}

void logVgmInfo()
{
    Serial.print("rom_song bytes: ");
    Serial.println(static_cast<unsigned long>(rom_song_size));
    Serial.print("VGM version: 0x");
    Serial.println(static_cast<unsigned long>(g_vgmPlayer.version()), HEX);
    Serial.print("Data offset: 0x");
    Serial.println(static_cast<unsigned long>(g_vgmPlayer.dataStartOffset()), HEX);
    Serial.print("Loop offset: 0x");
    Serial.println(static_cast<unsigned long>(g_vgmPlayer.loopOffset()), HEX);
}

void runIrqSelfTest()
{
    Serial.println("Testing YM2612 IRQ...");
    g_irqTestCounter = 0;
    attachInterrupt(digitalPinToInterrupt(kYmIrqPin), irqTestIsr, FALLING);
    g_ym2612.setTimerA(0);

    const unsigned long startedAt = millis();
    while (g_irqTestCounter < kIrqSelfTestPassCount) {
        if (millis() - startedAt > kIrqSelfTestTimeoutMs) {
            detachInterrupt(digitalPinToInterrupt(kYmIrqPin));
            g_ym2612.clearTimerA();
            Serial.println("DANGER!!! YM2612/YM3438 NOT DETECTED! POWER DOWN AND REMOVE IC!");
            while (true) {
                digitalWrite(kUserLedPin, HIGH);
                delay(100);
                digitalWrite(kUserLedPin, LOW);
                delay(100);
            }
        }
    }

    detachInterrupt(digitalPinToInterrupt(kYmIrqPin));
    g_ym2612.clearTimerA();
    Serial.println("YM IRQ OK!");
}

} // namespace

void setup()
{
    Serial.begin(115200);
    delay(1500);
    pinMode(kUserLedPin, OUTPUT);
    digitalWrite(kUserLedPin, LOW);
    pinMode(kYmIrqPin, INPUT);

    Serial.println();
    Serial.println("RE1-YM2612 minimal bus bring-up");
    logPinMapping();

    g_ym2612.begin();
    Serial.println("YM2612 reset complete.");

    g_ym2612.initializeSafeDefaults();
    Serial.println("YM2612 safe defaults applied.");

    runIrqSelfTest();

    if (!g_vgmPlayer.begin()) {
        Serial.println("Failed to parse rom_song VGM header.");
        return;
    }

    logVgmInfo();
    Serial.println("VGM playback started.");
}

void loop()
{
    const unsigned long now = millis();
    if (now - g_lastBlinkMillis >= kBlinkIntervalMs) {
        g_lastBlinkMillis = now;
        g_ledState = !g_ledState;
        digitalWrite(kUserLedPin, g_ledState ? HIGH : LOW);
    }

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
