#include <Arduino.h>

constexpr uint8_t LED_RED_PIN = 15;
constexpr uint8_t LED_WHITE_PIN = 16;
constexpr uint8_t LED_BLUE_PIN = 17;

constexpr unsigned long LED_RED_INTERVAL = 200;
constexpr unsigned long LED_WHITE_INTERVAL = 500;
constexpr unsigned long LED_WHITE_INTERVAL = 1000;

unsigned long ledRedPreviousMillis = 0;
unsigned long ledWhitePreviousMillis = 0;
unsigned long ledBluePreviousMillis = 0;

bool led1State = LOW;
bool led2State = LOW;
bool led3State = LOW;

void setup()
{
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_WHITE_PIN, OUTPUT);
    pinMode(LED_BLUE_PIN, OUTPUT);

    digitalWrite(LED_RED_PIN, led1State);
    digitalWrite(LED_WHITE_PIN, led2State);
    digitalWrite(LED_BLUE_PIN, led3State);
}

void loop()
{
    unsigned long currentMillis = millis();

    if (currentMillis - ledRedPreviousMillis >= LED_RED_INTERVAL)
    {
        ledRedPreviousMillis = currentMillis;

        led1State = !led1State;
        digitalWrite(LED_RED_PIN, led1State);
    }

    if (currentMillis - ledWhitePreviousMillis >= LED_WHITE_INTERVAL)
    {
        ledWhitePreviousMillis = currentMillis;

        led2State = !led2State;
        digitalWrite(LED_WHITE_PIN, led2State);
    }

    if (currentMillis - ledBluePreviousMillis >= LED_BLUE_PIN)
    {
        ledBluePreviousMillis = currentMillis;

        led3State = !led3State;
        digitalWrite(LED_BLUE_PIN, led3State);
    }
}