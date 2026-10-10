#include <Arduino.h>
#include <SPI.h>

static const int W5500_CS = 10;
static const int W5500_RST = 9;
static const int W5500_SCK = 13;
static const int W5500_MISO = 12;
static const int W5500_MOSI = 11;

uint8_t readW5500(uint16_t address)
{
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(W5500_CS, LOW);
  SPI.transfer((uint8_t)(address >> 8));
  SPI.transfer((uint8_t)(address & 0xFF));
  SPI.transfer(0x00); // common-register block, read, variable-length mode
  uint8_t value = SPI.transfer(0x00);
  digitalWrite(W5500_CS, HIGH);
  SPI.endTransaction();
  return value;
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  pinMode(W5500_RST, OUTPUT);
  digitalWrite(W5500_RST, LOW);
  delay(100);
  digitalWrite(W5500_RST, HIGH);
  delay(500);
  pinMode(W5500_CS, OUTPUT);
  digitalWrite(W5500_CS, HIGH);
  SPI.begin(W5500_SCK, W5500_MISO, W5500_MOSI, W5500_CS);
  Serial.println("W5500 SPI diagnostic: VERSIONR at 0x0039 should be 0x04");
}

void loop()
{
  uint8_t version = readW5500(0x0039);
  Serial.printf("VERSIONR = 0x%02X (%s)\n", version, version == 0x04 ? "chip responds" : "no valid W5500 response");
  delay(2000);
}
