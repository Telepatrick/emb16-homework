#include <Arduino.h>

constexpr uint8_t POT_PIN = 16;
constexpr uint8_t MOTOR_PIN = 17;

constexpr uint32_t PWM_PERIOD = 10;

uint32_t pwmStart = 0;
uint32_t highTime = 0;

void setup()
{
    pinMode(MOTOR_PIN, OUTPUT);
}

void loop()
{
    const uint32_t now = millis();
    const int adc = analogRead(POT_PIN);

    highTime = map(adc, 0, 4095, 0, PWM_PERIOD);

    const uint32_t phase = (now - pwmStart) % PWM_PERIOD;

    digitalWrite(
        MOTOR_PIN,
        phase < highTime ? HIGH : LOW
    );
}