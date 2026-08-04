#include <Arduino.h>

constexpr uint8_t BUTTON_PIN = 15;

constexpr unsigned long DEBOUNCE_DELAY = 0;

int16_t buttonCounter = 0;

bool lastReading = HIGH;
bool stableState = HIGH;

unsigned long lastDebounceTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  bool currentReading = digitalRead(BUTTON_PIN);

  if (currentReading != lastReading) {
    lastDebounceTime = millis();
  }

  if (millis() - lastDebounceTime >= DEBOUNCE_DELAY) {
    if (currentReading != stableState) {
      stableState = currentReading;

      if (stableState == LOW) {
        buttonCounter++;

        Serial.printf("Button Pressed! Count: %d\n",buttonCounter);
      }
    }
  }

  lastReading = currentReading;
}