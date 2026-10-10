#include <ModbusMaster.h>

#define RXD2 15
#define TXD2 16

ModbusMaster node;

// Convert two 16-bit Modbus registers to
// MFM376 Mid-Little-Endian (C-D-A-B) float
float readFloatCDAB(uint16_t reg1, uint16_t reg2)
{
  uint16_t high = node.getResponseBuffer(reg1);
  uint16_t low  = node.getResponseBuffer(reg2);

  uint32_t raw =
      ((uint32_t)high << 16) |
      low;

  // Rearrange ABCD -> CDAB
  uint32_t cdab =
      ((raw & 0x0000FFFF) << 16) |
      ((raw & 0xFFFF0000) >> 16);

  float value;
  memcpy(&value, &cdab, sizeof(value));

  return value;
}

void setup()
{
  Serial.begin(115200);

  // ESP32 UART2
  // RX = GPIO15
  // TX = GPIO16
  // 9600 baud, 8N1
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // MFM376 Slave ID = 1
  node.begin(1, Serial2);

  Serial.println();
  Serial.println("======================================");
  Serial.println("     SELEC MFM376 MODBUS TEST");
  Serial.println("======================================");
  Serial.println("Baud Rate : 9600");
  Serial.println("Slave ID  : 1");
  Serial.println("Format    : 8N1");
  Serial.println();
}

void loop()
{
  /*
     MFM376 register map:

     30000 = Voltage V1N
     30002 = Voltage V2N
     30004 = Voltage V3N
     30008 = Voltage V12
     30010 = Voltage V23
     30012 = Voltage V31

     30016 = Current I1
     30018 = Current I2
     30020 = Current I3

     30024 = kW1
     30026 = kW2
     30028 = kW3

     30030 = kVAr1
     30032 = kVAr2
     30034 = kVAr3

     30036 = kVA1
     30038 = kVA2
     30040 = kVA3

     30042 = PF1
     30044 = PF2
     30046 = PF3

     30048 = Average PF
     30050 = Frequency
     30052 = Total kW
     30054 = Total kVAR
     30056 = Total kVA
  */

  // Read registers 30000 through 30057
  // MFM376 register 30000 corresponds to address 0
  uint8_t result = node.readInputRegisters(0, 58);

  if (result == node.ku8MBSuccess)
  {
    Serial.println();
    Serial.println("========== METER DATA ==========");

    // Voltage
    float V1N = readFloatCDAB(0, 1);
    float V2N = readFloatCDAB(2, 3);
    float V3N = readFloatCDAB(4, 5);

    float V12 = readFloatCDAB(8, 9);
    float V23 = readFloatCDAB(10, 11);
    float V31 = readFloatCDAB(12, 13);

    // Current
    float I1 = readFloatCDAB(16, 17);
    float I2 = readFloatCDAB(18, 19);
    float I3 = readFloatCDAB(20, 21);

    // Power
    float kW1 = readFloatCDAB(24, 25);
    float kW2 = readFloatCDAB(26, 27);
    float kW3 = readFloatCDAB(28, 29);

    // Power factor
    float PF1 = readFloatCDAB(42, 43);
    float PF2 = readFloatCDAB(44, 45);
    float PF3 = readFloatCDAB(46, 47);

    float AvgPF = readFloatCDAB(48, 49);

    // Frequency
    float Frequency = readFloatCDAB(50, 51);

    // Total power
    float TotalKW   = readFloatCDAB(52, 53);
    float TotalKVAR = readFloatCDAB(54, 55);
    float TotalKVA  = readFloatCDAB(56, 57);

    // Print values
    Serial.println();

    Serial.println("--- VOLTAGE ---");

    Serial.print("V1N : ");
    Serial.print(V1N, 2);
    Serial.println(" V");

    Serial.print("V2N : ");
    Serial.print(V2N, 2);
    Serial.println(" V");

    Serial.print("V3N : ");
    Serial.print(V3N, 2);
    Serial.println(" V");

    Serial.print("V12 : ");
    Serial.print(V12, 2);
    Serial.println(" V");

    Serial.print("V23 : ");
    Serial.print(V23, 2);
    Serial.println(" V");

    Serial.print("V31 : ");
    Serial.print(V31, 2);
    Serial.println(" V");


    Serial.println();
    Serial.println("--- CURRENT ---");

    Serial.print("I1 : ");
    Serial.print(I1, 3);
    Serial.println(" A");

    Serial.print("I2 : ");
    Serial.print(I2, 3);
    Serial.println(" A");

    Serial.print("I3 : ");
    Serial.print(I3, 3);
    Serial.println(" A");


    Serial.println();
    Serial.println("--- POWER ---");

    Serial.print("kW1 : ");
    Serial.print(kW1, 3);
    Serial.println(" kW");

    Serial.print("kW2 : ");
    Serial.print(kW2, 3);
    Serial.println(" kW");

    Serial.print("kW3 : ");
    Serial.print(kW3, 3);
    Serial.println(" kW");

    Serial.print("Total kW : ");
    Serial.print(TotalKW, 3);
    Serial.println(" kW");

    Serial.print("Total kVAR : ");
    Serial.print(TotalKVAR, 3);
    Serial.println(" kVAR");

    Serial.print("Total kVA : ");
    Serial.print(TotalKVA, 3);
    Serial.println(" kVA");


    Serial.println();
    Serial.println("--- POWER FACTOR ---");

    Serial.print("PF1 : ");
    Serial.println(PF1, 3);

    Serial.print("PF2 : ");
    Serial.println(PF2, 3);

    Serial.print("PF3 : ");
    Serial.println(PF3, 3);

    Serial.print("Average PF : ");
    Serial.println(AvgPF, 3);


    Serial.println();
    Serial.println("--- FREQUENCY ---");

    Serial.print("Frequency : ");
    Serial.print(Frequency, 2);
    Serial.println(" Hz");

    Serial.println("================================");
  }
  else
  {
    Serial.print("Modbus Error Code: ");
    Serial.println(result);
  }

  delay(2000);
}