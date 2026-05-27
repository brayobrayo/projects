#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#include <MPU6050_tockn.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "MAX30105.h"
#include "heartRate.h"

#define MODEM_TX 19
#define MODEM_RX 18
#define SDA 5
#define SCL 4
#define buzzer 10
#define SerialAT Serial1

const char* ssid = "wifi";
const char* password = "0112507198";
const char Number[] = "+254787352127"; 
const char message[] = "Hello there, your loved one has experienced a fall. Attention needed";
const float FALL_THRESHOLD = 0.4;     // g units
const float IMPACT_THRESHOLD = 2.5;
const long IR_FINGER_THRESHOLD = 15000;
const byte RATE_SIZE = 8;
long lastBeat = 0;
float bpm = 0;
int bpmAvg = 0;
float tempC = 0;
bool fingerOn = false;
unsigned long lastSampleMs = 0;
unsigned long lastTempMs = 0;

MAX30105 particleSensor;
TinyGsm modem(SerialAT);
MPU6050 mpu(Wire);
byte rateBuffer[RATE_SIZE];
byte rateIndex = 0;

WebServer server(80);
String webpage = R"rawliteral(

<!DOCTYPE html>
<html>

<head>
    <title>Heart rate monitor</title>

    <meta name="viewport" content="width=device-width, initial-scale=1">

    <style>

        body{
            background:#111;
            color:white;
            font-family:Arial;
            text-align:center;
            padding-top:40px;
        }

        .card{
            background:#1e1e1e;
            width:300px;
            margin:auto;
            padding:25px;
            border-radius:15px;
            box-shadow:0 0 15px rgba(255,255,255,0.1);
        }

        h1{
            color:#00ffcc;
        }

        .value{
            font-size:45px;
            margin:15px;
            color:#00ffcc;
        }

        .label{
            font-size:20px;
            color:#bbb;
        }

        .status{
            font-size:24px;
            margin-top:20px;
        }

    </style>
</head>

<body>

    <div class="card">

        <h1>Heart rate monitor</h1>

        <div class="label">Heart Rate</div>
        <div class="value" id="bpm">0</div>

        <div class="label">Average BPM</div>
        <div class="value" id="avg">0</div>

        <div class="label">Temperature</div>
        <div class="value" id="temp">0</div>

        <div class="status" id="finger">NO FINGER</div>

    </div>

<script>

function updateData(){

    fetch("/data")
    .then(response => response.json())
    .then(data => {

        document.getElementById("bpm").innerHTML = data.bpm;
        document.getElementById("avg").innerHTML = data.avg;
        document.getElementById("temp").innerHTML = data.temp + " °C";

        if(data.finger){
            document.getElementById("finger").innerHTML = "FINGER DETECTED";
        }
        else{
            document.getElementById("finger").innerHTML = "NO FINGER";
        }

    });
}

setInterval(updateData,1000);

</script>

</body>
</html>

)rawliteral";
void handleRoot() {
    server.send(200, "text/html", webpage);
}
void handleData() {

    StaticJsonDocument<200> doc;

    doc["bpm"] = (int)bpm;
    doc["avg"] = bpmAvg;
    doc["temp"] = String(tempC,1);
    doc["finger"] = fingerOn;

    String json;
    serializeJson(doc, json);

    server.send(200, "application/json", json);
}
void setup() {

  Serial.begin(115200);
  delay(1000);
  //wifi conncetion
  Serial.println("Setting up Access Point..."); 
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point IP Address: ");
  Serial.println(IP);
   //sserial and 12C communication starting
  Serial.println("Starting...");
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  Wire.begin(SDA, SCL);
  delay(3000);
  //starting gsm
  Serial.println("Testing AT...");
  SerialAT.println("AT");
  delay(1000);
  while (SerialAT.available()) {
    Serial.write(SerialAT.read());
  }
  Serial.println("Restarting modem...");
  modem.restart();
  delay(3000);
//starting mpu
  Serial.println("Initializing MPU...");

 mpu.begin();
  mpu.calcGyroOffsets(true);

  Serial.println("MPU6050 Ready!");
 //gsm network conncetion 
 Serial.println("Waiting for network...");
  int reg = 0;
  for (int i = 0; i < 30; i++) {
    reg = modem.getRegistrationStatus();
    if (reg == 1 || reg == 5) break;
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  
  if (reg != 1 && reg != 5) {
    Serial.println("Network registration failed!");
    while (true) {
  Serial.println("Retrying network...");
  delay(2000);
} 
  } 
  Serial.println("Registered on network!"); 

  int csq = modem.getSignalQuality();
  Serial.print("Signal: ");
  Serial.println(csq);
//starting max sensor
if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {

        Serial.println("MAX30102 NOT FOUND");

        while (1);
    }

    particleSensor.setup(
        60,
        4,
        2,
        100,
        411,
        4096
    );

    particleSensor.setPulseAmplitudeRed(0x1F);
    particleSensor.setPulseAmplitudeIR(0x1F);
    particleSensor.setPulseAmplitudeGreen(0);

    tempC = particleSensor.readTemperature();
    //starting server
    server.on("/", handleRoot);
    server.on("/data", handleData);

    server.begin();

    Serial.println("Web Server Started");
   pinMode(buzzer, OUTPUT);
}

void loop() {

  server.handleClient();

  unsigned long now = millis();

  if (now - lastSampleMs >= 10) {

    lastSampleMs = now;

    long irValue = particleSensor.getIR();

    fingerOn = (irValue > IR_FINGER_THRESHOLD);

    if (fingerOn) {

      if (checkForBeat(irValue)) {

        long delta = now - lastBeat;
        lastBeat = now;

        float beatsPerMinute = 60.0 / (delta / 1000.0);

        if (beatsPerMinute > 40 && beatsPerMinute < 220) {

          bpm = beatsPerMinute;

          rateBuffer[rateIndex++] = (byte)bpm;
          rateIndex %= RATE_SIZE;

          long total = 0;

          for (byte i = 0; i < RATE_SIZE; i++) {
            total += rateBuffer[i];
          }

          bpmAvg = total / RATE_SIZE;

          Serial.print("BPM: ");
          Serial.println(bpmAvg);
        }
      }

    } else {

      bpm = 0;
      bpmAvg = 0;
    }
  }

  if (now - lastTempMs >= 5000) {

    lastTempMs = now;

    tempC = particleSensor.readTemperature();

    Serial.print("Temp: ");
    Serial.println(tempC);
  }

  mpu.update();

  float ax = mpu.getAccX();
  float ay = mpu.getAccY();
  float az = mpu.getAccZ();

  float total_accel = sqrt(ax * ax + ay * ay + az * az);

  Serial.print("Accel: ");
  Serial.println(total_accel);

  if (total_accel < FALL_THRESHOLD) {

    Serial.println("FREE FALL DETECTED!");

    unsigned long start = millis();

    while (millis() - start < 500) {

      mpu.update();

      ax = mpu.getAccX();
      ay = mpu.getAccY();
      az = mpu.getAccZ();

      float impact = sqrt(ax * ax + ay * ay + az * az);

      if (impact > IMPACT_THRESHOLD) {

        Serial.println("IMPACT DETECTED!");

        triggerAlert();

        break;
      }

      delay(5);
    }
  }

  delay(5);
}
//alert mechanism
void triggerAlert() {
  Serial.println("!!! FALL CONFIRMED !!!");
  //buzzer on
  digitalWrite (buzzer, HIGH);
   delay(3000);
   digitalWrite(buzzer, LOW);
  
  //send sms
  if (modem.sendSMS(Number, message)) {
    Serial.println("SMS sent successfully!");
  } else {
    Serial.println("SMS failed to send.");}
  //make a call
   Serial.println("calling");
  if (modem.callNumber(Number)) {
    Serial.println("Call connected!");
    delay(30000); 
    modem.callHangup();
    Serial.println("Call ended.");
  } else {
    Serial.println("Call failed!");
  }
}

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
