#include <Wire.h>
#include <Adafruit_MCP4725.h>

Adafruit_MCP4725 dac;

const int bpm = 120;
const unsigned long interval = 60000 / bpm;
unsigned long previousMillis = 0;

// Toggle this to switch between tick (downbeat) and tock (upbeat)
bool isAccent = true;

void setup() {
    Wire.begin();
    Wire.setClock(400000);
    dac.begin(0x60);
    dac.setVoltage(2048, false);
    randomSeed(analogRead(A0)); // Seed for noise generation
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        playClick(isAccent);
        // isAccent = !isAccent; // Uncomment for alternating tick/tock
    }
}

void playClick(bool accent) {
    int duration = 100;
    float filtered = 2048.0;

    for (int i = 0; i < duration; i++) {
        float amplitude = exp(-0.05 * i);
        float noise = (random(0, 2048) - 1024) / 1024.0;
        float target = 2048.0 + (2047.0 * amplitude * noise);
        filtered = filtered + 0.25 * (target - filtered); // Heavy filtering
        dac.setVoltage((uint16_t)constrain(filtered, 0, 4095), false);
        delayMicroseconds(40);
    }
    dac.setVoltage(2048, false);
}

