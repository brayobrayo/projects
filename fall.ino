#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <LiquidCrystal.h>
Adafruit_MPU6050 mpu;
const float FALL_THRESHOLD = 31.0;  // Adjust based on testing (g)
const int FALL_DURATION = 500;     // Duration to confirm fall (ms)
unsigned long fallStartTime = 0;
bool falling = false;
void setup() {
  Serial.begin(115200);
  delay(2000);
  // Initialize I2C (use GPIO21 and GPIO22 for ESP32 unless you wired differently)
  Wire.begin(19, 18);  // SDA=19, SCL=18 (Check your wiring!)
  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU-6050 chip");
    while (1) delay(10);
  Serial.println("MPU6050 Found!");
  delay(1000);
void loop() {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);
  // Calculate total acceleration (magnitude)
  float totalAccel = sqrt(
    accel.acceleration.x * accel.acceleration.x +
    accel.acceleration.y * accel.acceleration.y +
    accel.acceleration.z * accel.acceleration.z
  );
  Serial.print("Accel: ");
  Serial.println(totalAccel);  // Show acceleration for debugging
  // Check for possible fall
  if (totalAccel < FALL_THRESHOLD) {
    if (!falling) {
      falling = true;
      fallStartTime = millis();
    } 
    else if (millis() - fallStartTime > FALL_DURATION) {
      
      Serial.println("FALL DETECTED");
      delay(2000);  // Show message for 2 seconds
     
      falling = false;  // Reset
    }
  } else {
    // Normal state
    if (falling) {
      falling = false;  // Reset if acceleration returns
    }
  delay(2000);  
