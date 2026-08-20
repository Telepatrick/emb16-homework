#include <Arduino.h>

constexpr uint8_t RELAY_PIN = 17;
constexpr uint8_t CONTACT_PIN = 16;

constexpr uint8_t RELAY_ON = HIGH;
constexpr uint8_t RELAY_OFF = LOW;

constexpr uint8_t MEASUREMENTS_COUNT = 10;
constexpr uint32_t MEASUREMENT_TIMEOUT = 1000;
constexpr uint32_t RELAY_SETTLE_TIME = 500;

volatile bool contactTriggered = false;
volatile uint32_t interruptTime = 0;

uint32_t measurements[MEASUREMENTS_COUNT];
uint8_t measurementIndex = 0;
uint32_t measurementStartTime = 0;
uint32_t relayOffTime = 0;

bool measurementInProgress = false;
bool finished = false;

void IRAM_ATTR contactISR()
{
    if (!contactTriggered)
    {
        interruptTime = millis();
        contactTriggered = true;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    pinMode(RELAY_PIN, OUTPUT);
    pinMode(CONTACT_PIN, INPUT_PULLUP);

    digitalWrite(RELAY_PIN, RELAY_OFF);

    attachInterrupt(
        digitalPinToInterrupt(CONTACT_PIN),
        contactISR,
        RISING
    );

    relayOffTime = millis();

    Serial.println("| Measurement | Response time (ms)|");

    Serial.println("|   :---:     |       :---:       |");
}

void startMeasurement()
{
    contactTriggered = false;
    interruptTime = 0;
    measurementStartTime = millis();
    measurementInProgress = true;

    digitalWrite(RELAY_PIN, RELAY_ON);
}

void finishMeasurement()
{
    uint32_t triggerTime = interruptTime;
    uint32_t responseTime = triggerTime - measurementStartTime;

    measurements[measurementIndex] = responseTime;
    measurementIndex++;

    Serial.printf(
        "| %11u | %17u |\n",
        measurementIndex,
        responseTime);

    measurementInProgress = false;

    digitalWrite(RELAY_PIN, RELAY_OFF);

    relayOffTime = millis();

    contactTriggered = false;

    if (measurementIndex >= MEASUREMENTS_COUNT)
    {
        uint32_t sum = 0;

        for (uint8_t i = 0; i < MEASUREMENTS_COUNT; i++)
        {
            sum += measurements[i];
        }

        float average = static_cast<float>(sum) / MEASUREMENTS_COUNT;

        Serial.println();
        Serial.print("Average response time: ");
        Serial.print(average, 2);
        Serial.println(" ms");

        finished = true;
    }
}

void loop()
{
    if (finished)
        return;

    if (measurementInProgress)
    {
        if (contactTriggered)
            finishMeasurement();

        return;
    }

    if (measurementIndex >= MEASUREMENTS_COUNT)
        return;

    if (millis() - relayOffTime >= RELAY_SETTLE_TIME)
        startMeasurement();
}