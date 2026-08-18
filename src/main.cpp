#include <Arduino.h>

constexpr uint8_t LED_RED_PIN = 15;
constexpr uint8_t LED_WHITE_PIN = 16;
constexpr uint8_t LED_BLUE_PIN = 17;

constexpr unsigned long LED_RED_INTERVAL = 200;
constexpr unsigned long LED_WHITE_INTERVAL = 500;
constexpr unsigned long LED_BLUE_INTERVAL = 1000;

unsigned long ledRedPreviousMillis = 0;
unsigned long ledWhitePreviousMillis = 0;
unsigned long ledBluePreviousMillis = 0;

bool ledRedState = LOW;
bool ledWhiteState = LOW;
bool ledBlueState = LOW;

void setup()
{
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_WHITE_PIN, OUTPUT);
    pinMode(LED_BLUE_PIN, OUTPUT);

    digitalWrite(LED_RED_PIN, ledRedState);
    digitalWrite(LED_WHITE_PIN, ledWhiteState);
    digitalWrite(LED_BLUE_PIN, ledBlueState);
}

void loop()
{
    unsigned long currentMillis = millis();

    if (currentMillis - ledRedPreviousMillis >= LED_RED_INTERVAL)
    {
        ledRedPreviousMillis = currentMillis;

        ledRedState = !ledRedState;
        digitalWrite(LED_RED_PIN, ledRedState);
    }

    if (currentMillis - ledWhitePreviousMillis >= LED_WHITE_INTERVAL)
    {
        ledWhitePreviousMillis = currentMillis;

        ledWhiteState = !ledWhiteState;
        digitalWrite(LED_WHITE_PIN, ledWhiteState);
    }

    if (currentMillis - ledBluePreviousMillis >= LED_BLUE_PIN)
    {
        ledBluePreviousMillis = currentMillis;

        ledBlueState = !ledBlueState;
        digitalWrite(LED_BLUE_PIN, ledBlueState);
    }
}