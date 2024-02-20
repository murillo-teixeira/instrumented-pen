#include <WiFi.h>
#include "wifi_credentials.h"

// WiFi credentials
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

// TCP
WiFiClient client;
const char *host = RECEIVER_IP; // Replace with your computer's IP address
const int port = 4210;          // The port number should match the one used in your server script

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ; // Wait for Serial to be ready
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

    // Connect to TCP server
    Serial.print("Connecting to server...");
    if (!client.connect(host, port))
    {
        Serial.println("Connection to server failed");
    }
    Serial.println("Connected to server");
}

void loop()
{
    float data[9] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0}; // Example data
    // Send the data directly, no need to copy into a separate buffer for TCP
    if (client.connected())
    {
        client.write((uint8_t *)data, sizeof(data));
    }
    else
    {
        Serial.println("Connection to server failed");
        // Attempt to reconnect
        if (!client.connect(host, port))
        {
            Serial.println("Reconnection failed");
            delay(5000); // Wait 5 seconds before retrying
        }
        else
        {
            Serial.println("Reconnected to server");
        }
    }

    delay(10); // Adjust based on how frequently you want to send data
}
