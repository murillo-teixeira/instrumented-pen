#include "Led.h"
#include "Arduino.h"

Led::Led(int pinRed, int pinGreen, int pinBlue)
{
    _pinRed = pinRed;
    _pinGreen = pinGreen;
    _pinBlue = pinBlue;
    pinMode(_pinRed, OUTPUT);
    pinMode(_pinGreen, OUTPUT);
    pinMode(_pinBlue, OUTPUT);
    off();
    _previousMillis = 0;
    _ledState = false;
}

void Led::blink(LedColor color, unsigned long delayTime)
{
    _currentColor = color;
    _delayTime = delayTime;
}

void Led::update()
{
    unsigned long currentMillis = millis();
    if (currentMillis - _previousMillis >= _delayTime)
    {
        _previousMillis = currentMillis;
        if (_ledState)
        {
            off();
            _ledState = false;
        }
        else
        {
            switch (_currentColor)
            {
            case RED:
                analogWrite(_pinRed, 5);
                break;
            case GREEN:
                analogWrite(_pinGreen, 5);
                break;
            case BLUE:
                analogWrite(_pinBlue, 5);
                break;
            }
            _ledState = true;
        }
    }
}

void Led::off()
{
    analogWrite(_pinRed, 0);
    analogWrite(_pinGreen, 0);
    analogWrite(_pinBlue, 0);
}
