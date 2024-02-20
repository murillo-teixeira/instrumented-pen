#include <WiFi.h>
#include <WiFiUdp.h>
#include "wifi_credentials.h"

// WiFi credentials
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

// UDP
WiFiUDP udp;
const char *udpAddress = RECEIVER_IP; // Replace with your computer's IP address
const int udpPort = 4210;             // Choose an appropriate port number

void setup()
{
    Serial.begin(115200);
    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Initialize UDP
    udp.begin(udpPort);
}

void loop()
{
    float data[9] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0}; // Example data
    uint8_t buffer[sizeof(data)];                                  // Create a buffer to hold the bytes of the floats

    // Copy the floats into the buffer
    memcpy(buffer, data, sizeof(data));

    // Send the buffer
    udp.beginPacket(udpAddress, udpPort);
    udp.write(buffer, sizeof(buffer));
    udp.endPacket();

    delay(10); // Send at 100Hz
}