void setup() {
  Serial.begin(115200);
  delay(1000);

  float lat = -1.2921;   // South → negative
  float lng = 36.8219;   // East → positive

  String googleMapsLink = "https://maps.google.com/?q=";
  googleMapsLink += String(lat, 6);
  googleMapsLink += ",";
  googleMapsLink += String(lng, 6);

  Serial.println(googleMapsLink);
}

void loop() {
}
