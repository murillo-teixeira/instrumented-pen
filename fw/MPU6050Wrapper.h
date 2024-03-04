// MPU6050Wrapper.h

#ifndef MPU6050Wrapper_h
#define MPU6050Wrapper_h

#include <Arduino.h>
#include "MPU6050_6Axis_MotionApps612.h"

class MPU6050Wrapper
{
public:
    MPU6050Wrapper(uint8_t interruptPin);
    void initialize();
    bool isDataAvailable();
    void getOrientation(float *ypr, float *accel, float *gyro);

private:
    MPU6050 mpu;
    uint8_t interruptPin;
    static volatile bool mpuInterrupt;
    bool dmpReady = false;
    uint8_t mpuIntStatus;
    uint8_t devStatus;
    uint16_t packetSize;
    uint8_t fifoBuffer[64];

    Quaternion q;
    VectorInt16 aa;
    VectorInt16 gy;
    VectorFloat gravity;
    float euler[3];

    int currentAccelRange = MPU6050_ACCEL_FS_4;
    int currentGyroRange = MPU6050_GYRO_FS_500;

    static void dmpDataReadyStatic();

    float getAccelResolution();
    float getGyroResolution();
};

#endif
