#include <Arduino.h>

constexpr uint8_t ADC_RESOLUTION = 12;
constexpr uint16_t ADC_MAX = (1 << ADC_RESOLUTION) - 1;
constexpr uint8_t VOLTAGE_PIN = 4;
constexpr uint16_t READING_DELAY = 3000;

void setup() {
    Serial.begin(9600);
    delay(1000);

    analogReadResolution(ADC_RESOLUTION);
    analogSetPinAttenuation(VOLTAGE_PIN, ADC_11db);

	Serial.println("| Raw Data | U calc (mV) | U read (mV) | Error (%) |");
	Serial.println("|  :---:   |    :---:    |    :---:    |   :---:   |");
}

void loop() {
    uint16_t rawValue = analogRead(VOLTAGE_PIN);
    uint32_t millivolts = analogReadMilliVolts(VOLTAGE_PIN);

    float uCalc = (static_cast<float>(rawValue) / ADC_MAX) * 3300.0f;
    float measurError = ((uCalc - millivolts) / millivolts) * 100;

    Serial.printf(
        "|   %4d   | %11.2f | %10lu  | %+9.2f |\n",
        rawValue, uCalc, millivolts, measurError
    );

    delay(READING_DELAY);
}