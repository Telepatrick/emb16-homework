#include <Arduino.h>

constexpr uint8_t RED_LED_PIN = 15;
constexpr uint8_t BLUE_LED_PIN = 16;

constexpr uint8_t EXTERN_BUTTON_PIN = 21;
constexpr uint8_t BOOT_BUTTON_PIN = 0;

constexpr uint16_t MIN_BLINK_DELAY = 50;
constexpr uint16_t MAX_BLINK_DELAY = 1000;
constexpr uint16_t BLINK_STEP = 100;
constexpr uint16_t DEBOUNCE_TIME = 50;
constexpr uint16_t LONG_PRESS_TIME = 1000;

uint16_t blinkDelay = 200;
bool alternativeMode = false;

uint8_t externalLastReading = HIGH;
uint8_t externalStableState = HIGH;
unsigned long externalLastDebounceTime = 0;
unsigned long externalPressStartTime = 0;
bool externalLongPressHandled = false;

uint8_t bootLastReading = HIGH;
uint8_t bootStableState = HIGH;
unsigned long bootLastDebounceTime = 0;

bool ledState = LOW;

uint8_t currentBlink = 0;

unsigned long lastBlinkTime = 0;

void initHardware();
void handleButtons();
bool wasButtonPressed(uint8_t pin, uint8_t &lastReading, uint8_t &stableState, unsigned long &lastDebounceTime);
void handleBlinking();

void setup() {
  Serial.begin(115200);
  initHardware();
  Serial.printf("Blink delay: %u ms\n", blinkDelay);
}

void loop() {
  handleButtons();
  handleBlinking();
}

void initHardware() {
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(BLUE_LED_PIN, LOW);
  
  pinMode(EXTERN_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
}

void handleButtons() {
  if (
    wasButtonPressed(
      EXTERN_BUTTON_PIN,
      externalLastReading,
      externalStableState,
      externalLastDebounceTime
    )
  ) {
    if (blinkDelay + BLINK_STEP <= MAX_BLINK_DELAY)
      blinkDelay += BLINK_STEP;

    Serial.printf("External button: blink delay = %u ms\n", blinkDelay);
  }

  if (
    wasButtonPressed(
      BOOT_BUTTON_PIN,
      bootLastReading,
      bootStableState,
      bootLastDebounceTime
    )
  ) {
    if (blinkDelay >= MIN_BLINK_DELAY + BLINK_STEP)
      blinkDelay -= BLINK_STEP;

    Serial.printf("BOOT button: blink delay = %u ms\n", blinkDelay);
  }
}

bool wasButtonPressed(
    uint8_t pin,
    uint8_t &lastReading,
    uint8_t &stableState,
    unsigned long &lastDebounceTime
) {
  uint8_t currentReading = digitalRead(pin);

  if (currentReading != lastReading) {
    lastDebounceTime = millis();
    lastReading = currentReading;
  }

  if (millis() - lastDebounceTime >= DEBOUNCE_TIME) {
    if (currentReading != stableState) {
      stableState = currentReading;

      if (stableState == LOW) {
        if (pin == EXTERN_BUTTON_PIN) {
          externalPressStartTime = millis();
          externalLongPressHandled = false;
        } else {
          return true;
        }
      } else if (
        pin == EXTERN_BUTTON_PIN
        && !externalLongPressHandled
      ) {
        return true;
      }
    }

    if (
      pin == EXTERN_BUTTON_PIN
      && stableState == LOW
      && !externalLongPressHandled
      && millis() - externalPressStartTime >= LONG_PRESS_TIME
    ) {
      alternativeMode = !alternativeMode;
      externalLongPressHandled = true;

      Serial.printf("Alternative mode: %s\n", alternativeMode ? "ON" : "OFF");
    }
  }

  return false;
}

void handleBlinking() {
  if (millis() - lastBlinkTime >= blinkDelay) {
    ledState = !ledState;

    digitalWrite(RED_LED_PIN, ledState);
    digitalWrite(BLUE_LED_PIN, alternativeMode ? !ledState : ledState);

    lastBlinkTime = millis();
  }
}