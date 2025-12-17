#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

// Thresholds in m/s^2 (Standard Gravity is ~9.8 m/s^2)
const float FALL_THRESHOLD = 4.0;    // Triggered when accel drops (weightlessness)
const float IMPACT_THRESHOLD = 25.0; // Triggered on sudden stop (> ~2.5G)

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C (Pin 4 = SDA, Pin 5 = SCL for ESP8266/ESP32)
  Wire.begin(4, 5);

  // Correct initialization check
  if (!mpu.begin(0x68)) {
    Serial.println("Failed to find MPU6500 chip");
    while (1) yield();
  }
  
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("MPU6500 Ready!");
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Total Acceleration Magnitude = sqrt(x^2 + y^2 + z^2)
  float total_accel = sqrt(pow(a.acceleration.x, 2) + 
                           pow(a.acceleration.y, 2) + 
                           pow(a.acceleration.z, 2));

  // 1. Detect Free Fall (magnitude drops below threshold)
  if (total_accel < FALL_THRESHOLD) {
    Serial.println("FREE FALL DETECTED!");
    
    // 2. Check for Impact within the next 1000ms
    unsigned long start = millis();
    while (millis() - start < 1000) {
      mpu.getEvent(&a, &g, &temp);
      float impact_accel = sqrt(pow(a.acceleration.x, 2) + 
                                pow(a.acceleration.y, 2) + 
                                pow(a.acceleration.z, 2));
      
      if (impact_accel > IMPACT_THRESHOLD) {
        Serial.print("IMPACT DETECTED: ");
        Serial.println(impact_accel);
        triggerAlert();
        break; 
      }
    }
  }
  delay(50); // Faster sampling for better detection
}

void triggerAlert() {
  Serial.println("!!! FALL CONFIRMED !!!");
  // Add buzzer or SMS logic here
}
