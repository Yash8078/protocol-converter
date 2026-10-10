#include <ModbusMaster.h>

#define RXD2 15
#define TXD2 16

ModbusMaster node;

void setup() {
  Serial.begin(115200);

  // 9600 baud, 8 data bits, No parity, 1 stop bit
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // MFM376 Slave ID = 1
  node.begin(1, Serial2);

  Serial.println("==============================");
  Serial.println("MFM376 Modbus Test");
  Serial.println("Baud Rate : 9600");
  Serial.println("Slave ID  : 1");
  Serial.println("Format    : 8N1");
  Serial.println("==============================");
}

void loop() {

  uint8_t result = node.readInputRegisters(0, 2);

  if (result == node.ku8MBSuccess) {

    Serial.println("METER RESPONSE RECEIVED!");

    Serial.print("Register 0 = ");
    Serial.println(node.getResponseBuffer(0));

    Serial.print("Register 1 = ");
    Serial.println(node.getResponseBuffer(1));

  } else {

    Serial.print("Modbus Error = ");
    Serial.println(result);
  }

  delay(2000);
}