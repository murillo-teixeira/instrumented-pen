float getAccelResolution(int range)
{
    switch (range)
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

float getGyroResolution(int range)
{
    switch (range)
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
