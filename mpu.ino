#include <MPU6050_tockn.h>
#include <Wire.h>

const int SDA_PIN = 4;
const int SCL_PIN = 5;

MPU6050 mpu(Wire);

// Thresholds
const float FALL_THRESHOLD = 0.4;     // g units
const float IMPACT_THRESHOLD = 2.5;   // g units

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);
  delay(2000);

  Serial.println("Initializing MPU...");

  mpu.begin();
  mpu.calcGyroOffsets(true);

  Serial.println("MPU6050 Ready!");
}

void loop() {

  mpu.update();

  float ax = mpu.getAccX();
  float ay = mpu.getAccY();
  float az = mpu.getAccZ();

  float total_accel = sqrt(ax*ax + ay*ay + az*az);

  Serial.print("Accel: ");
  Serial.println(total_accel);

  // Detect free fall
  if (total_accel < FALL_THRESHOLD) {

    Serial.println("FREE FALL DETECTED!");

    unsigned long start = millis();

    while (millis() - start < 1000) {

      mpu.update();

      ax = mpu.getAccX();
      ay = mpu.getAccY();
      az = mpu.getAccZ();

      float impact = sqrt(ax*ax + ay*ay + az*az);

      if (impact > IMPACT_THRESHOLD) {

        Serial.print("IMPACT DETECTED: ");
        Serial.println(impact);

        triggerAlert();
        break;
      }
    }
  }

  delay(50);
}

void triggerAlert() {
  Serial.println("!!! FALL CONFIRMED !!!");
}