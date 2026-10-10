#define TXD2 15

void setup() {
  Serial.begin(115200);

  Serial2.begin(9600, SERIAL_8N1, 16, TXD2);

  Serial.println("UART TX test started");
}

void loop() {
  Serial2.println("TEST");
  Serial.println("Sending TEST");
  delay(1000);
}