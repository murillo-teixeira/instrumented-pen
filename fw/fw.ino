#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps612.h"

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

#include "Menu.h"
#include "ImuHelpers.h"
#include "QuaternionOperations.h"
#include "wifi_credentials.h"

MPU6050 mpu;

#define INTERRUPT_PIN 3

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;        // [w, x, y, z]         quaternion container
VectorInt16 aa;      // [x, y, z]            accel sensor measurements
VectorInt16 gy;      // [x, y, z]            gyro sensor measurements
VectorInt16 aaReal;  // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld; // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity; // [x, y, z]            gravity vector
float euler[3];      // [psi, theta, phi]    Euler angle container
float ypr[3];        // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

int currentAccelRange = MPU6050_ACCEL_FS_4; // Default to ±4g
int currentGyroRange = MPU6050_GYRO_FS_500; // Default to ±500°/s

// Operation modes

enum Mode
{
    MODE_IDLE,
    MODE_ALL,
    MODE_WIFI,
    MODE_YAW_PITCH_ROLL,
    MODE_ACCELEROMETER,
    MODE_GYROSCOPE,
    MODE_SET_GYRO_RANGE,
    MODE_SET_ACCEL_RANGE
};

Mode currentMode = MODE_ALL; // Default mode
String currentCommand;
bool commandToProcess = false;

// Menu items

MenuItem mainMenu[] = {
    {'0', "Stream everything (UDP)\t\t\t\t([Enter] to exit)", []()
     { currentMode = MODE_WIFI; }},
    {'1', "Stream everything (Serial)\t\t\t\t([Enter] to exit)", []()
     { currentMode = MODE_ALL; }},
    {'2', "Stream accelerometer readings (g)\t\t([Enter] to exit)", []()
     { currentMode = MODE_ACCELEROMETER; }},
    {'3', "Stream gyroscope readings (dg/s)\t\t([Enter] to exit)", []()
     { currentMode = MODE_GYROSCOPE; }},
    {'4', "Stream yaw, pitch and roll readings (dg)\t([Enter] to exit)", []()
     { currentMode = MODE_YAW_PITCH_ROLL; }},
    {'5', "Get accel and gyro full scale ranges", []()
     {
         Serial.print("\n\tAFS_SEL: ");
         Serial.println(mpu.getFullScaleAccelRange());
         Serial.print("\tFS_SEL: ");
         Serial.println(mpu.getFullScaleGyroRange());
         Serial.print("\n[Enter] to exit");
     }},
    {'6', "Set accelerometer full scale range\t\t(Enter range after selection, [Enter] to exit)", []()
     {
         Serial.print("Enter the accelerometer full scale range (0-3): ");
         currentMode = MODE_SET_ACCEL_RANGE;
     }},
    {'7', "Set gyroscope full scale range\t\t(Enter range after selection, [Enter] to exit)", []()
     {
         Serial.print("Enter the gyroscope full scale range (0-3): ");
         currentMode = MODE_SET_GYRO_RANGE;
     }},
};

const size_t mainMenuSize = sizeof(mainMenu) / sizeof(mainMenu[0]);

// ===================================================
// ===               WIFI VARIABLES                ===
// ===================================================

// WiFi credentials
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

// UDP
WiFiUDP udp;
const char *udpAddress = RECEIVER_IP; // Replace with your computer's IP address
const int udpPort = 4210;             // Choose an appropriate port number

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================

volatile bool mpuInterrupt = false; // indicates whether MPU interrupt pin has gone high

void dmpDataReady()
{
    mpuInterrupt = true;
}

// ================================================================
// ===                      INITIAL SETUP                       ===
// ================================================================

void setup()
{
    // join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    Wire.begin();
    Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
    Fastwire::setup(400, true);
#endif

    // initialize serial communication
    // (115200 chosen because it is required for Teapot Demo output, but it's
    // really up to you depending on your project)
    Serial.begin(115200);
    while (!Serial)
        ; // wait for Leonardo enumeration, others continue immediately

    // NOTE: 8MHz or slower host processors, like the Teensy @ 3.3V or Arduino
    // Pro Mini running at 3.3V, cannot handle this baud rate reliably due to
    // the baud timing being too misaligned with processor ticks. You must use
    // 38400 or slower in these cases, or use some kind of external separate
    // crystal solution for the UART timer.

    // initialize device
    Serial.println(F("Initializing I2C devices..."));
    mpu.initialize();
    pinMode(INTERRUPT_PIN, INPUT);

    // verify connection
    Serial.println(F("Testing device connections..."));
    Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

    // wait for ready
    Serial.println(F("\nSend any character to begin DMP programming and demo: "));

    // load and configure the DMP
    Serial.println(F("Initializing DMP..."));
    devStatus = mpu.dmpInitialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    mpu.setXAccelOffset(-2080);
    mpu.setYAccelOffset(1643);
    mpu.setZAccelOffset(652);
    mpu.setXGyroOffset(-72);
    mpu.setYGyroOffset(-38);
    mpu.setZGyroOffset(-2);
    // make sure it worked (returns 0 if so)
    if (devStatus == 0)
    {
        // Calibration Time: generate offsets and calibrate our MPU6050
        // mpu.CalibrateAccel(6);
        // mpu.CalibrateGyro(6);
        Serial.println();
        mpu.PrintActiveOffsets();
        // turn on the DMP, now that it's ready
        Serial.println(F("Enabling DMP..."));
        mpu.setDMPEnabled(true);

        // enable Arduino interrupt detection
        Serial.print(F("Enabling interrupt detection (Arduino external interrupt "));
        Serial.print(digitalPinToInterrupt(INTERRUPT_PIN));
        Serial.println(F(")..."));
        attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
        mpuIntStatus = mpu.getIntStatus();

        // set our DMP Ready flag so the main loop() function knows it's okay to use it
        Serial.println(F("DMP ready!"));
        dmpReady = true;

        // get expected DMP packet size for later comparison
        packetSize = mpu.dmpGetFIFOPacketSize();

        mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_4);
        mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_500);

        delay(2000);
        displayMenu();

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
    else
    {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        Serial.print(F("DMP Initialization failed (code "));
        Serial.print(devStatus);
        Serial.println(F(")"));
        Serial.println("Restart the system to try again.");
    }
}

// ================================================================
// ===                    MAIN PROGRAM LOOP                     ===
// ================================================================

void loop()
{

    if (!dmpReady)
        return;

    if (currentMode == MODE_IDLE)
    {
        readSelection();
        return;
    }
    else
    {
        if (Serial.available() > 0)
        {
            currentCommand = Serial.readStringUntil('\n');
            char selection = currentCommand.charAt(0);
            if (currentCommand.length() == 1)
            {
                currentMode = MODE_IDLE;
                Serial.println("\nExiting to main menu...\n");
                flushSerialBuffer();
                displayMenu();
            }
            else
            {
                commandToProcess = true;
            }
        }
    }

    if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer))
    {
        Quaternion qRotated;
        switch (currentMode)
        {
        case MODE_ALL:
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            qRotated = rotateQuaternionAlongY(q, 90.0);
            mpu.dmpGetAccel(&aa, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &qRotated);
            mpu.dmpGetYawPitchRoll(ypr, &qRotated, &gravity);
            mpu.dmpGetGyro(&gy, fifoBuffer);

            Serial.print("yaw:");
            Serial.print(ypr[0] * 180 / M_PI);
            Serial.print(",\tpitch:");
            Serial.print(ypr[1] * 180 / M_PI);
            Serial.print(",\troll:");
            Serial.print(ypr[2] * 180 / M_PI);

            Serial.print(",\tacc_x:");
            Serial.print(aa.x * getAccelResolution(currentAccelRange));
            Serial.print(",\tacc_y:");
            Serial.print(aa.y * getAccelResolution(currentAccelRange));
            Serial.print(",\tacc_z:");
            Serial.print(aa.z * getAccelResolution(currentAccelRange));

            Serial.print(",\tgyro_x:");
            Serial.print(gy.x * getGyroResolution(currentGyroRange));
            Serial.print(",\tgyro_y:");
            Serial.print(gy.y * getGyroResolution(currentGyroRange));
            Serial.print(",\tgyro_z:");
            Serial.println(gy.z * getGyroResolution(currentGyroRange));

            break;

        case MODE_YAW_PITCH_ROLL:
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            qRotated = rotateQuaternionAlongY(q, 90.0);
            mpu.dmpGetAccel(&aa, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &qRotated);
            mpu.dmpGetYawPitchRoll(ypr, &qRotated, &gravity);

            Serial.print("yaw:");
            Serial.print(ypr[0] * 180 / M_PI);
            Serial.print(",\tpitch:");
            Serial.print(ypr[1] * 180 / M_PI);
            Serial.print(",\troll:");
            Serial.println(ypr[2] * 180 / M_PI);
            break;

        case MODE_ACCELEROMETER:
            mpu.dmpGetAccel(&aa, fifoBuffer);

            Serial.print("acc_x:");
            Serial.print(aa.x * getAccelResolution(currentAccelRange));
            Serial.print(",\tacc_y:");
            Serial.print(aa.y * getAccelResolution(currentAccelRange));
            Serial.print(",\tacc_z:");
            Serial.println(aa.z * getAccelResolution(currentAccelRange));
            break;
        case MODE_GYROSCOPE:
            mpu.dmpGetGyro(&gy, fifoBuffer);

            Serial.print("gyro_x:");
            Serial.print(gy.x * getGyroResolution(currentGyroRange));
            Serial.print(",\tgyro_y:");
            Serial.print(gy.y * getGyroResolution(currentGyroRange));
            Serial.print(",\tgyro_z:");
            Serial.println(gy.z * getGyroResolution(currentGyroRange));
            break;

        case MODE_SET_ACCEL_RANGE:
            if (commandToProcess)
            {
                // Never enter here...
                currentCommand.trim();

                if (currentCommand.length() != 1 || !isDigit(currentCommand[0]))
                {
                    Serial.println("\nInvalid input. Please enter a value between 0 and 3.");
                    Serial.print("\nEnter the accelerometer full scale range (0-3): ");
                    commandToProcess = false;
                }
                else
                {
                    int range = currentCommand.toInt();
                    if (range >= 0 && range <= 3)
                    {
                        mpu.setFullScaleAccelRange(range);
                        currentAccelRange = range;
                        Serial.print("\nAccelerometer full scale range set to: ");
                        Serial.println(range);
                        currentMode = MODE_IDLE;
                        flushSerialBuffer();
                        displayMenu();
                        commandToProcess = false;
                    }
                    else
                    {
                        Serial.println("\nInvalid range. Please enter a value between 0 and 3.");
                        Serial.print("\nEnter the accelerometer full scale range (0-3): ");
                        commandToProcess = false;
                    }
                }
            }
            break;
        default:
            break;
        }
    }
}