#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>

#define MODEM_TX 3 
#define MODEM_RX 2 
#define SerialAT Serial1

TinyGsm modem(SerialAT);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Starting...");
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);
  
  Serial.println("Testing AT...");
  SerialAT.println("AT");
  delay(1000);
  while (SerialAT.available()) {
    Serial.write(SerialAT.read());
  }
  
  Serial.println("Restarting modem...");
  modem.restart();
  delay(3000);
  
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
    return;
  }
  Serial.println("Registered on network!");

  int csq = modem.getSignalQuality();
  Serial.print("Signal: ");
  Serial.println(csq);

  Serial.println("Dialing +254787352127...");
  if (modem.callNumber("+254787352127")) {
    Serial.println("Call connected!");
    delay(30000); 
    modem.callHangup();
    Serial.println("Call ended.");
  } else {
    Serial.println("Call failed!");
  }
}

void loop (){
  
}