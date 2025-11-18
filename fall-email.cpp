#include <Adafruit_MPU6050.h>
#include <ESP_Mail_Client.h>
#include <WiFi.h>
#define WIFI_SSID "Brian"
#define WIFI_PASSWORD "0787352127"
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "brianmutiso970@gmail.com"
#define AUTHOR_PASSWORD "qiaaahmidyiumzpx"
#define RECIPIENT_EMAIL "brianmutiso@students.uonbi.ac.ke"
SMTPSession smtp;
Adafruit_MPU6050 mpu;
const int FALL_THRESHOLD = 9.0;  
const int FALL_DURATION = 100;    
unsigned long fallStartTime = 0;
bool falling = false;
void smtpCallback(SMTP_Status status);
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
    delay(1000);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print(F("Connecting to Wi-Fi"));
 while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);}
  Serial.println();
  Serial.print(F("Connected with IP: "));
  Serial.println(WiFi.localIP());
  Serial.println();
  /*  Set the network reconnection option */
  MailClient.networkReconnect(true); 
  smtp.debug(1);
 /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);
 /* Declare the Session_Config for user defined session credentials */
  Session_Config config;
  /* Set the session config */
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = "";
 config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;
  /* Declare the message class */
  SMTP_Message message;
  message.sender.name = F("ESP FALL DETECTION");
  message.sender.email = AUTHOR_EMAIL;
  message.addRecipient(F("WAMBUA BRIAN MUTISO"), RECIPIENT_EMAIL);
  message.subject = F("FALL DETECTED");
  String textMsg = "Hello there. your loved one has experienced a fall.";
  message.text.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;
  if (!smtp.connect(&config)){
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  if (!smtp.isLoggedIn()){
    Serial.println("\nNot yet logged in.");
  }
  else{
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }
  }
 }
void loop(){
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
     SMTP_Message message;
     if (!MailClient.sendMail(&smtp, &message, true))
    Serial.println("Error sending Email, " + smtp.errorReason());
      delay(2000);  // Show message for 2 seconds
     
      falling = false;  // Reset
    }
  } else {
    // Normal state
    if (falling) {
      falling = false;  // Reset if acceleration returns
    }
   
  }
  delay(2000);  // Sa
}
void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");
    
    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }  
}
