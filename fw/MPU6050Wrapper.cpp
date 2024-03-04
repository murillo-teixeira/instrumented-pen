// MPU6050Wrapper.cpp

#include "MPU6050Wrapper.h"
#include "I2Cdev.h"
#include "Wire.h"
#include "QuaternionOperations.h" // Make sure you have this if you're using quaternion operations

MPU6050Wrapper::MPU6050Wrapper(uint8_t interruptPin) : interruptPin(interruptPin), mpu(MPU6050()) {}

volatile bool MPU6050Wrapper::mpuInterrupt = false; // Correct initialization

void MPU6050Wrapper::initialize()
{
    Wire.begin();
    Wire.setClock(400000); // 400kHz I2C clock
    mpu.initialize();
    pinMode(interruptPin, INPUT);
    devStatus = mpu.dmpInitialize();
    mpu.setXAccelOffset(-2078);
    mpu.setYAccelOffset(-467);
    mpu.setZAccelOffset(-732);
    mpu.setXGyroOffset(-192);
    mpu.setYGyroOffset(-224);
    mpu.setZGyroOffset(-23);
    if (devStatus == 0)
    {
        mpu.setDMPEnabled(true);
        attachInterrupt(digitalPinToInterrupt(interruptPin), MPU6050Wrapper::dmpDataReadyStatic, RISING);
        mpuIntStatus = mpu.getIntStatus();
        dmpReady = true;
        packetSize = mpu.dmpGetFIFOPacketSize();

        mpu.setFullScaleAccelRange(currentAccelRange);
        mpu.setFullScaleGyroRange(currentGyroRange);
    }
    else
    {
        Serial.print(F("DMP Initialization failed (code "));
        Serial.print(devStatus);
        Serial.println(F(")"));
    }
}

bool MPU6050Wrapper::isDataAvailable()
{
    if (!dmpReady)
        return false;
    return mpu.dmpGetCurrentFIFOPacket(fifoBuffer);
}

void MPU6050Wrapper::getOrientation(float *ypr, float *accel, float *gyro)
{
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    // Quaternion qRotated = rotateQuaternionAlongY(q, 90.0); // Assuming rotateQuaternionAlongY() is defined elsewhere
    mpu.dmpGetAccel(&aa, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
    mpu.dmpGetGyro(&gy, fifoBuffer);
    ypr[0] = ypr[0] * 180 / M_PI;
    ypr[1] = ypr[1] * 180 / M_PI;
    ypr[2] = ypr[2] * 180 / M_PI;
    accel[0] = aa.x * getAccelResolution();
    accel[1] = aa.y * getAccelResolution();
    accel[2] = aa.z * getAccelResolution();
    gyro[0] = gy.x * getGyroResolution();
    gyro[1] = gy.y * getGyroResolution();
    gyro[2] = gy.z * getGyroResolution();
}

float MPU6050Wrapper::getAccelResolution()
{
    switch (currentAccelRange)
    {
    case MPU6050_ACCEL_FS_2:
        return 1.0 / 16384.0;
    case MPU6050_ACCEL_FS_4:
        return 1.0 / 8192.0;
    case MPU6050_ACCEL_FS_8:
        return 1.0 / 4096.0;
    case MPU6050_ACCEL_FS_16:
        return 1.0 / 2048.0;
    default:
        return 0; // Error case
    }
}

float MPU6050Wrapper::getGyroResolution()
{
    switch (currentGyroRange)
    {
    case MPU6050_GYRO_FS_250:
        return 1.0 / 131.0;
    case MPU6050_GYRO_FS_500:
        return 1.0 / 65.5;
    case MPU6050_GYRO_FS_1000:
        return 1.0 / 32.8;
    case MPU6050_GYRO_FS_2000:
        return 1.0 / 16.4;
    default:
        return 0; // Error case
    }
}

void MPU6050Wrapper::dmpDataReadyStatic()
{
    mpuInterrupt = true;
}
