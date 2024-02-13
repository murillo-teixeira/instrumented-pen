Quaternion multiplyQuaternions(Quaternion &q1, Quaternion &q2)
{
    Quaternion result;
    result.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
    result.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
    result.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
    result.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
    return result;
}

Quaternion rotateQuaternionAlongY(Quaternion &q, float angle_degrees)
{
    double halfAngleRad = 90 * M_PI / 180.0 / 2.0;
    Quaternion rotationQuaternion = {
        rotationQuaternion.w = cos(halfAngleRad),
        rotationQuaternion.x = sin(halfAngleRad),
        rotationQuaternion.y = 0,
        rotationQuaternion.z = 0};

    // Apply the rotation using quaternion multiplication
    return multiplyQuaternions(q, rotationQuaternion);
}
