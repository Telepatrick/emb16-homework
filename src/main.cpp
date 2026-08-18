#include <Arduino.h>

enum class LedState : uint8_t
{
    Off,
    On
};

class Config
{
public:
    static constexpr uint8_t LedPin = 15;
    static constexpr uint8_t ButtonPin = 16;

    static constexpr uint32_t BlinkIntervalMs = 500;

    static const uint8_t BlinksOnShortPress = 3;

    static constexpr uint32_t SerialReportIterations = 1000;
};

class Led
{
public:
    explicit Led(uint8_t pin)
        : pin_(pin)
    {
    }

    void init()
    {
        pinMode(pin_, OUTPUT);
        set(LedState::Off);
    }

    void set(LedState state)
    {
        state_ = state;

        digitalWrite(
            pin_,
            state_ == LedState::On ? HIGH : LOW
        );
    }

    LedState state() const
    {
        return state_;
    }

private:
    const uint8_t pin_;
    LedState state_{LedState::Off};
};

enum class LedMode : uint8_t
{
    Blinking,
    ConstantOn,
    ConstantOff
};

class Application
{
public:
    void init()
    {
        led_.init();

        pinMode(
            Config::ButtonPin,
            INPUT_PULLUP
        );

        attachInterrupt(
            digitalPinToInterrupt(Config::ButtonPin),
            buttonInterrupt,
            FALLING
        );

        lastBlinkTime_ = millis();
        lastLoopTime_ = micros();

        Serial.begin(115200);
    }

    void update()
    {
        processButton();

        const uint32_t currentTime = millis();

        if (mode_ == LedMode::Blinking)
        {
            updateBlink(currentTime);
        }

        measureLoopTime();
    }

private:
    Led led_{Config::LedPin};

    LedMode mode_{LedMode::Blinking};

    uint32_t lastBlinkTime_{0};

    uint32_t lastLoopTime_{0};
    uint32_t totalLoopTime_{0};
    uint32_t loopCounter_{0};

    static volatile bool buttonPressed_;

    void updateBlink(uint32_t currentTime)
    {
        if (currentTime - lastBlinkTime_
            < Config::BlinkIntervalMs)
        {
            return;
        }

        lastBlinkTime_ = currentTime;

        if (led_.state() == LedState::On)
        {
            led_.set(LedState::Off);
        }
        else
        {
            led_.set(LedState::On);
        }
    }

    void processButton()
    {
        bool pressed = false;
        noInterrupts();

        if (buttonPressed_)
        {
            buttonPressed_ = false;
            pressed = true;
        }

        interrupts();

        if (!pressed)
        {
            return;
        }

        switch (mode_)
        {
            case LedMode::Blinking:
                mode_ = LedMode::ConstantOn;
                led_.set(LedState::On);

                Serial.println(
                    "Mode: constant ON"
                );
                break;

            case LedMode::ConstantOn:
                mode_ = LedMode::ConstantOff;
                led_.set(LedState::Off);

                Serial.println(
                    "Mode: constant OFF"
                );
                break;

            case LedMode::ConstantOff:
                mode_ = LedMode::Blinking;
                lastBlinkTime_ = millis();

                Serial.println(
                    "Mode: blinking"
                );
                break;
        }
    }

    void measureLoopTime()
    {
        const uint32_t currentTime = micros();
        const uint32_t iterationTime =
            currentTime - lastLoopTime_;

        lastLoopTime_ = currentTime;

        totalLoopTime_ += iterationTime;
        ++loopCounter_;

        if (loopCounter_>= Config::SerialReportIterations)
        {
            const uint32_t averageTime =
                totalLoopTime_ / loopCounter_;

            Serial.print("Iterations: ");
            Serial.print(loopCounter_);

            Serial.print(", average loop time: ");
            Serial.print(averageTime);

            Serial.println(" us");

            loopCounter_ = 0;
            totalLoopTime_ = 0;
        }
    }

    static void buttonInterrupt()
    {
        buttonPressed_ = true;
    }
};

volatile bool Application::buttonPressed_ = false;

Application application;

void setup()
{
    application.init();
}

void loop()
{
    application.update();
}