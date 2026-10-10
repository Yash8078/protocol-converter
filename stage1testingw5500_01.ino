#include <SPI.h>
#include <Ethernet.h>

// W5500 pins
#define W5500_MOSI 11
#define W5500_MISO 12
#define W5500_SCK  13
#define W5500_CS   10
#define W5500_RST  9

byte mac[] = {
  0x02, 0xAA, 0xBB, 0xCC, 0xDD, 0x01
};

// Static IP for W5500
IPAddress ip(192, 168, 10, 50);
IPAddress dns(192, 168, 10, 1);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);

EthernetServer server(80);

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("==============================");
  Serial.println("ESP32-S3 + W5500 WEB SERVER");
  Serial.println("==============================");

  // Reset W5500
  pinMode(W5500_RST, OUTPUT);
  digitalWrite(W5500_RST, LOW);
  delay(100);
  digitalWrite(W5500_RST, HIGH);
  delay(500);

  // Start SPI
  SPI.begin(W5500_SCK, W5500_MISO, W5500_MOSI, W5500_CS);

  // W5500 CS
  Ethernet.init(W5500_CS);

  Serial.println("Starting W5500...");

  // Start Ethernet with static IP
  Ethernet.begin(mac, ip, dns, gateway, subnet);

  delay(1000);

  Serial.print("IP Address: ");
  Serial.println(Ethernet.localIP());

  Serial.print("Subnet: ");
  Serial.println(Ethernet.subnetMask());

  Serial.print("Gateway: ");
  Serial.println(Ethernet.gatewayIP());

  Serial.print("Link status: ");

  if (Ethernet.linkStatus() == LinkON) {
    Serial.println("LINK ON");
  } else {
    Serial.println("LINK OFF");
  }

  // Start web server
  server.begin();

  Serial.println("Web server started!");
  Serial.println("Open this address in your laptop browser:");
  Serial.println("http://192.168.10.50");
  Serial.println("==============================");
}

void loop() {

  EthernetClient client = server.available();

  if (client) {

    Serial.println("Browser connected!");

    bool currentLineIsBlank = true;

    while (client.connected()) {

      if (client.available()) {

        char c = client.read();

        if (c == '\n' && currentLineIsBlank) {

          // Send HTTP response
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();

          client.println("<!DOCTYPE html>");
          client.println("<html>");
          client.println("<head>");
          client.println("<title>ESP32-S3 W5500</title>");
          client.println("</head>");

          client.println("<body>");
          client.println("<h1>ESP32-S3 + W5500</h1>");
          client.println("<h2>Ethernet connection working!</h2>");
          client.println("<p>Web server is running successfully.</p>");
          client.println("</body>");

          client.println("</html>");

          break;
        }

        if (c == '\n') {
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }

    delay(1);
    client.stop();

    Serial.println("Browser disconnected.");
  }
}
