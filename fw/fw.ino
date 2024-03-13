#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "MPU6050Wrapper.h"
#include "Led.h"
#include "wifi_credentials.h"

// LED object
#define LED_B D8
#define LED_G D9
#define LED_R D10

Led led(LED_R, LED_G, LED_B);

// Button
#define BUTTON_PIN D1
unsigned long buttonPressStartTime = 0; // Time when the button was pressed
bool readyForDeepSleep = false;         // Flag to indicate ready for deep sleep

// Variables for MPU6050
float ypr[3];   // Yaw, Pitch, Roll
float accel[3]; // Acceleration X, Y, Z
float gyro[3];  // Gyroscopic X, Y, Z

// MPU6050 object
#define MPU_INTERRUPT_PIN D3
MPU6050Wrapper mpuWrapper(MPU_INTERRUPT_PIN);

// WiFi credentials
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
bool connecting_to_wifi = false;

// UDP
WiFiUDP udp;
const char *udpAddress = RECEIVER_IP; // Replace with your computer's IP address
const int udpPort = 4210;             // Choose an appropriate port number
char incomingPacket[255];             // Buffer for incoming packets

// PVT Test
bool pvtStarted = false;
float pvtDelay;
float pvtStartTime;
float pvtEndTime;

// Operation modes
enum Mode
{
    MODE_SETUP_WIFI,
    MODE_STREAM,
    MODE_DEBUG,
};

Mode currentMode = MODE_SETUP_WIFI;

//////////////////////////////////////////////
///////////////// SETUP //////////////////////
//////////////////////////////////////////////

void setup()
{
    delay(1000);
    Serial.begin(115200);
    Serial.println("Connected to serial");

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    esp_deep_sleep_enable_gpio_wakeup((1ULL << 3), ESP_GPIO_WAKEUP_GPIO_LOW);

    mpuWrapper.initialize();
}

/////////////////////////////////////////////
///////////////// LOOP //////////////////////
/////////////////////////////////////////////

void loop()
{
    switch (currentMode)
    {
    case MODE_SETUP_WIFI:
        if (!connecting_to_wifi)
        {
            WiFi.disconnect();
            WiFi.begin(ssid, password);
            connecting_to_wifi = true;

            led.blink(Led::BLUE, 500); // Blink red every 500 ms
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            led.update();
        }
        else
        {
            udp.begin(udpPort);
            currentMode = MODE_STREAM;
            Serial.println(WiFi.localIP().toString());
            led.off();
        }
        break;
    case MODE_STREAM:
        if (WiFi.status() != WL_CONNECTED)
        {
            currentMode = MODE_SETUP_WIFI;
            connecting_to_wifi = false;
            break;
        }
        else
        {
            streamData();

            if (pvtStarted)
            {
                if (!digitalRead(BUTTON_PIN))
                { // Assuming a LOW signal on button press
                    pvtEndTime = millis();
                    float reactionTime = pvtEndTime - pvtStartTime - pvtDelay;
                    sendPVTData(reactionTime);
                    pvtStarted = false; // Reset PVT
                    led.off();
                }
                else if (millis() - pvtStartTime > pvtDelay)
                {
                    led.on(Led::RED);
                }
            }
            else
            {
                checkForUDPMessages();
            }
        }
        break;
    default:
        break;
    }

    handleButtonPress();

    delay(1);
}

void streamData()
{
    static unsigned long lastMPUUpdateTime = 0;
    static unsigned long lastBatteryUpdateTime = 0;
    unsigned long currentMillis = millis();

    // Stream MPU6050 data at 100Hz
    if (currentMillis - lastMPUUpdateTime >= 10)
    {
        lastMPUUpdateTime = currentMillis;
        if (mpuWrapper.isDataAvailable())
        {
            mpuWrapper.getOrientation(ypr, accel, gyro);

            float data[10] = {currentMillis, ypr[0], ypr[1], ypr[2], accel[0], accel[1], accel[2], gyro[0], gyro[1], gyro[2]};
            uint8_t messageType = 0x01;
            uint8_t buffer[1 + sizeof(data)];
            buffer[0] = messageType;
            memcpy(&buffer[1], data, sizeof(data));

            udp.beginPacket(udpAddress, udpPort);
            udp.write(buffer, sizeof(buffer));
            udp.endPacket();
        }
    }

    // Send battery voltage at 0.5Hz
    if (currentMillis - lastBatteryUpdateTime >= 2000)
    {
        lastBatteryUpdateTime = currentMillis;

        float Vbatt = getBatteryVoltage();
        uint8_t messageType = 0x02;
        uint8_t buffer[1 + sizeof(float)];
        buffer[0] = messageType;
        memcpy(&buffer[1], &Vbatt, sizeof(Vbatt));

        udp.beginPacket(udpAddress, udpPort);
        udp.write(buffer, sizeof(buffer));
        udp.endPacket();
    }
}

void checkForUDPMessages()
{
    int packetSize = udp.parsePacket();
    if (packetSize)
    {
        int len = udp.read(incomingPacket, 255);
        if (len > 0)
        {
            incomingPacket[len] = 0; // Null-terminate the string
        }
        if (String(incomingPacket) == "PVT")
        {
            pvtStarted = true;
            pvtStartTime = millis();
            pvtDelay = random(5000, 10000); // Random delay between 2 and 5 seconds
            led.off();                      // Ensure the LED is off when the PVT starts        }
        }
    }
}

void sendPVTData(float reactionTime)
{
    uint8_t messageType = 0x03; // Assuming 0x03 signifies PVT data
    uint8_t buffer[1 + sizeof(float)];
    buffer[0] = messageType;
    memcpy(&buffer[1], &reactionTime, sizeof(reactionTime));

    udp.beginPacket(udpAddress, udpPort);
    udp.write(buffer, sizeof(buffer));
    udp.endPacket();
}

float getBatteryVoltage()
{
    uint32_t Vbatt = 0;
    for (int i = 0; i < 16; i++)
    {
        Vbatt += analogReadMilliVolts(A2);
    }
    float Vbattf = 2 * Vbatt / 16 / 1000.0;
    return Vbattf;
}

void handleButtonPress()
{
    if (!digitalRead(BUTTON_PIN))
    { // If the button is pressed
        if (buttonPressStartTime == 0)
        {                                    // If this is the start of the press
            buttonPressStartTime = millis(); // Record press start time
        }
        else if (millis() - buttonPressStartTime > 5000)
        {                             // If pressed for more than 5 seconds
            readyForDeepSleep = true; // Set the flag indicating ready for deep sleep
        }
    }
    else
    { // If the button is not pressed
        if (readyForDeepSleep)
        { // If the button was pressed long enough and now released
            Serial.println("Entering deep sleep mode...");
            esp_deep_sleep_start(); // Enter deep sleep mode
        }
        // Reset the start time and the flag regardless of whether deep sleep was triggered
        buttonPressStartTime = 0;
        readyForDeepSleep = false;
    }
}