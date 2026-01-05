#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;
float totalAccel;
const float FALL_THRESHOLD = 9.0;  // Adjust based on testing (g)
const int FALL_DURATION = 10;     // Duration to confirm fall (ms)
unsigned long fallStartTime = 0;
bool falling = false;

void setup(){
  Serial.begin(115200);
  delay(2000);
  Wire.begin(4, 5);
   if (!mpu.begin(0x68)<0) { 
    Serial.println(F("Could not find MPU6050 chip!"));
   }
    else {
    Serial.println(F("MPU6050 Found!"));
    }
    
    // Configure MPU6050 settings
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    
    Serial.println(F("MPU6050 Configured!"));
}

void loop() {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);
  Serial.println("ACCEL X");
  Serial.print(accel.acceleration.x);
  Serial.println("ACCEL Y");
  Serial.print(accel.acceleration.y);
  Serial.println("ACCEL Z");
  Serial.print(accel.acceleration.z);

totalAccel =  sqrt ((accel.acceleration.x * accel.acceleration.x) + 
                    (accel.acceleration.y * accel.acceleration.y) + 
                    (accel.acceleration.z * accel.acceleration.z));
//Serial.print("total acceleration:");
//Serial.println(totalAccel);
Serial.print("GYRO X");  
Serial.println(gyro.gyro.x);
Serial.print("GYRO Y");
Serial.println(gyro.gyro.y);
Serial.print("GYRO Z");
Serial.println(gyro.gyro.z);

delay(1000);

}
