#ifndef Led_h
#define Led_h

#include "Arduino.h"

class Led
{
public:
    enum LedColor
    {
        RED,
        GREEN,
        BLUE
    };

    Led(int pinRed, int pinGreen, int pinBlue);
    void blink(LedColor color, unsigned long delayTime);
    void update();
    void off();
    void on(LedColor color);

private:
    int _pinRed, _pinGreen, _pinBlue;
    unsigned long _previousMillis;
    unsigned long _delayTime;
    bool _ledState;
    LedColor _currentColor;
};

#endif
