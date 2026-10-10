#include <Arduino.h>
#include <ETH.h>
#include <SPI.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_system.h>

// =====================================================
//                    W5500 CONFIG
// =====================================================

#define W5500_CS       10
#define W5500_SCK      13
#define W5500_MISO     12
#define W5500_MOSI     11

// The existing W5500 hardware test uses GPIO 9 for reset.
#define W5500_INT      -1
#define W5500_RST      9

// ESP32-S3 SPI2
#define W5500_SPI_HOST SPI2_HOST

// =====================================================
//                    RS485 CONFIG
// =====================================================

#define RS485_RX_PIN   16
#define RS485_TX_PIN   15
#define RS485_DE_PIN   17

HardwareSerial RS485(1);

// =====================================================
//                    WEB SERVER
// =====================================================

WebServer server(80);

// Change these before uploading the firmware.
static const char WEB_USERNAME[] = "admin";
static const char WEB_PASSWORD[] = "change-this-before-use";
static const unsigned long SESSION_TIMEOUT_MS = 15UL * 60UL * 1000UL;

String activeSessionToken;
unsigned long sessionLastActivity = 0;
uint8_t failedLoginAttempts = 0;
unsigned long loginBlockedUntil = 0;
const char *REQUEST_HEADERS[] = {"Cookie"};

// =====================================================
//                    TCP SERVER
// =====================================================

#define DEFAULT_TCP_PORT 5000

NetworkServer tcpServer(DEFAULT_TCP_PORT);
NetworkClient tcpClient;

// =====================================================
//                    SETTINGS
// =====================================================

Preferences prefs;

uint32_t baudRate = 9600;

uint8_t stopBits = 1;

char parityMode = 'N';

uint16_t tcpPort = DEFAULT_TCP_PORT;

// =====================================================
//                    NETWORK
// =====================================================

bool ethConnected = false;

bool dhcpEnabled = false;

IPAddress localIP(
    192, 168, 1, 100
);

IPAddress gateway(
    192, 168, 1, 1
);

IPAddress subnet(
    255, 255, 255, 0
);

IPAddress dns(
    8, 8, 8, 8
);

// =====================================================
//                    DATA COUNTERS
// =====================================================

unsigned long rs485RxBytes = 0;
unsigned long rs485TxBytes = 0;

unsigned long tcpRxBytes = 0;
unsigned long tcpTxBytes = 0;

// =====================================================
//                    LAST DATA
// =====================================================

String lastRS485RX = "";
String lastRS485TX = "";

String lastTCPRX = "";
String lastTCPTX = "";

// =====================================================
//                    SERIAL CONFIG
// =====================================================

uint32_t getSerialConfig()
{
    if (parityMode == 'E')
    {
        if (stopBits == 2)
            return SERIAL_8E2;

        return SERIAL_8E1;
    }

    if (parityMode == 'O')
    {
        if (stopBits == 2)
            return SERIAL_8O2;

        return SERIAL_8O1;
    }

    // No parity
    if (stopBits == 2)
        return SERIAL_8N2;

    return SERIAL_8N1;
}

// =====================================================
//                    HEX CONVERSION
// =====================================================

String bufferToHex(
    const uint8_t *buffer,
    size_t length
)
{
    String result = "";

    for (size_t i = 0; i < length; i++)
    {
        if (buffer[i] < 0x10)
            result += "0";

        result += String(
            buffer[i],
            HEX
        );

        if (i < length - 1)
            result += " ";
    }

    result.toUpperCase();

    return result;
}

// =====================================================
//                    LOAD SETTINGS
// =====================================================

void loadSettings()
{
    prefs.begin(
        "config",
        true
    );

    baudRate = prefs.getUInt(
        "baud",
        9600
    );

    parityMode = prefs.getChar(
        "parity",
        'N'
    );

    stopBits = prefs.getUChar(
        "stop",
        1
    );

    tcpPort = prefs.getUShort(
        "tcp",
        DEFAULT_TCP_PORT
    );

    prefs.end();

    Serial.println();
    Serial.println("Loaded configuration:");

    Serial.print("Baud: ");
    Serial.println(baudRate);

    Serial.print("Parity: ");
    Serial.println(parityMode);

    Serial.print("Stop bits: ");
    Serial.println(stopBits);

    Serial.print("TCP port: ");
    Serial.println(tcpPort);
}

// =====================================================
//                    START RS485
// =====================================================

void startRS485()
{
    digitalWrite(
        RS485_DE_PIN,
        LOW
    );

    RS485.end();

    delay(50);

    RS485.begin(
        baudRate,
        getSerialConfig(),
        RS485_RX_PIN,
        RS485_TX_PIN
    );

    Serial.println();
    Serial.println("================================");
    Serial.println("RS485 INITIALIZED");
    Serial.println("================================");

    Serial.print("RX GPIO : ");
    Serial.println(RS485_RX_PIN);

    Serial.print("TX GPIO : ");
    Serial.println(RS485_TX_PIN);

    Serial.print("DE GPIO : ");
    Serial.println(RS485_DE_PIN);

    Serial.print("Baud    : ");
    Serial.println(baudRate);

    Serial.print("Parity  : ");
    Serial.println(parityMode);

    Serial.print("Stop    : ");
    Serial.println(stopBits);
}

// =====================================================
//                    START ETHERNET
// =====================================================

bool startEthernet()
{
    Serial.println();
    Serial.println("================================");
    Serial.println("STARTING W5500");
    Serial.println("================================");

    // Match the reset pulse used by the standalone W5500 hardware test.
    pinMode(W5500_RST, OUTPUT);
    digitalWrite(W5500_RST, LOW);
    delay(100);
    digitalWrite(W5500_RST, HIGH);
    delay(500);

    // ETH.begin() configures the SPI bus using the pins below.

    bool result = ETH.begin(
        ETH_PHY_W5500,
        1,
        W5500_CS,
        W5500_INT,
        W5500_RST,
        W5500_SPI_HOST,
        W5500_SCK,
        W5500_MISO,
        W5500_MOSI,
        1 // MHz: use the slower diagnostic clock for reliable SPI reads
    );

    if (!result)
    {
        Serial.println();
        Serial.println("ERROR: ETH.begin() FAILED");

        return false;
    }

    Serial.println();
    Serial.println("ETH.begin() SUCCESSFUL");

    // -------------------------------------------------
    // STATIC IP
    // -------------------------------------------------

    if (!dhcpEnabled)
    {
        bool configOK = ETH.config(
            localIP,
            gateway,
            subnet,
            dns
        );

        if (configOK)
        {
            Serial.println(
                "Static IP configured"
            );
        }
        else
        {
            Serial.println(
                "Static IP configuration FAILED"
            );
        }
    }

    return true;
}

// =====================================================
//                    NETWORK STATUS
// =====================================================

void updateEthernetStatus()
{
    if (ETH.linkUp())
    {
        if (!ethConnected)
        {
            ethConnected = true;

            Serial.println();
            Serial.println("================================");
            Serial.println("ETHERNET LINK UP");
            Serial.println("================================");

            Serial.print("IP      : ");
            Serial.println(
                ETH.localIP()
            );

            Serial.print("Subnet  : ");
            Serial.println(
                ETH.subnetMask()
            );

            Serial.print("Gateway : ");
            Serial.println(
                ETH.gatewayIP()
            );

            Serial.print("MAC     : ");
            Serial.println(
                ETH.macAddress()
            );
        }
    }
    else
    {
        if (ethConnected)
        {
            ethConnected = false;

            Serial.println();
            Serial.println("ETHERNET LINK DOWN");
        }
    }
}

// =====================================================
//                    TCP CLIENT
// =====================================================

void checkTCPClient()
{
    if (tcpClient && tcpClient.connected())
        return;

    NetworkClient newClient =
        tcpServer.accept();

    if (newClient)
    {
        tcpClient = newClient;

        Serial.println();
        Serial.println("================================");
        Serial.println("TCP CLIENT CONNECTED");
        Serial.println("================================");

        Serial.print(
            "Client IP: "
        );

        Serial.println(
            tcpClient.remoteIP()
        );
    }
}

// =====================================================
//                    RS485 -> TCP
// =====================================================

void rs485ToTCP()
{
    if (!tcpClient ||
        !tcpClient.connected())
    {
        return;
    }

    uint8_t buffer[256];

    size_t count = 0;

    while (
        RS485.available() &&
        count < sizeof(buffer)
    )
    {
        buffer[count++] =
            RS485.read();
    }

    if (count == 0)
        return;

    tcpClient.write(
        buffer,
        count
    );

    tcpClient.flush();

    rs485RxBytes += count;

    tcpTxBytes += count;

    lastRS485RX =
        bufferToHex(
            buffer,
            count
        );

    lastTCPTX =
        lastRS485RX;

    Serial.print(
        "RS485 -> TCP : "
    );

    Serial.println(
        lastRS485RX
    );
}

// =====================================================
//                    TCP -> RS485
// =====================================================

void tcpToRS485()
{
    if (!tcpClient ||
        !tcpClient.connected())
    {
        return;
    }

    uint8_t buffer[256];

    size_t count = 0;

    while (
        tcpClient.available() &&
        count < sizeof(buffer)
    )
    {
        buffer[count++] =
            tcpClient.read();
    }

    if (count == 0)
        return;

    lastTCPRX =
        bufferToHex(
            buffer,
            count
        );

    tcpRxBytes += count;

    Serial.print(
        "TCP -> RS485 : "
    );

    Serial.println(
        lastTCPRX
    );

    // -------------------------------------------------
    // ENABLE RS485 TRANSMIT
    // -------------------------------------------------

    digitalWrite(
        RS485_DE_PIN,
        HIGH
    );

    delayMicroseconds(100);

    RS485.write(
        buffer,
        count
    );

    RS485.flush();

    delayMicroseconds(100);

    // -------------------------------------------------
    // RETURN TO RECEIVE MODE
    // -------------------------------------------------

    digitalWrite(
        RS485_DE_PIN,
        LOW
    );

    rs485TxBytes += count;

    lastRS485TX =
        lastTCPRX;
}

// =====================================================
//                    WEB PAGE
// =====================================================

String makePage()
{
    String html = "";

    html += "<!DOCTYPE html>";

    html += "<html>";

    html += "<head>";

    html +=
        "<meta name='viewport' "
        "content='width=device-width,initial-scale=1'>";

    html +=
        "<title>ESP32 Protocol Converter</title>";

    html += "<style>";

    html +=
        "body{"
        "font-family:Arial;"
        "margin:0;"
        "background:#f2f4f7;"
        "}";

    html +=
        ".header{"
        "background:#0879c9;"
        "color:white;"
        "padding:18px;"
        "font-size:22px;"
        "}";

    html +=
        ".container{"
        "max-width:1000px;"
        "margin:auto;"
        "padding:20px;"
        "}";

    html +=
        ".card{"
        "background:white;"
        "padding:20px;"
        "margin-bottom:20px;"
        "border-radius:8px;"
        "box-shadow:0 2px 8px #ccc;"
        "}";

    html +=
        "input,select{"
        "width:100%;"
        "padding:10px;"
        "margin-top:5px;"
        "margin-bottom:15px;"
        "box-sizing:border-box;"
        "}";

    html +=
        "button{"
        "background:#0879c9;"
        "color:white;"
        "border:0;"
        "padding:12px 20px;"
        "border-radius:5px;"
        "cursor:pointer;"
        "}";

    html +=
        ".green{color:green;font-weight:bold;}";

    html +=
        ".red{color:red;font-weight:bold;}";

    html +=
        "pre{"
        "background:#111;"
        "color:#00ff66;"
        "padding:15px;"
        "overflow:auto;"
        "min-height:20px;"
        "}";

    html += "</style>";

    html += "</head>";

    html += "<body>";

    // =================================================
    // HEADER
    // =================================================

    html +=
        "<div class='header'>"
        "ESP32 RS485 Ethernet Protocol Converter"
        "</div>";

    html +=
        "<form action='/logout' method='POST' "
        "style='position:absolute;right:18px;top:16px'>"
        "<input type='hidden' name='csrf' value='" +
        activeSessionToken +
        "'><button type='submit'>Log out</button></form>";

    html +=
        "<div class='container'>";

    // =================================================
    // CURRENT STATUS
    // =================================================

    html += "<div class='card'>";

    html += "<h2>Current Status</h2>";

    html += "<p>Ethernet: ";

    if (ethConnected)
        html +=
            "<span class='green'>CONNECTED</span>";
    else
        html +=
            "<span class='red'>DISCONNECTED</span>";

    html += "</p>";

    html +=
        "<p><b>IP Address:</b> " +
        ETH.localIP().toString() +
        "</p>";

    html +=
        "<p><b>Gateway:</b> " +
        ETH.gatewayIP().toString() +
        "</p>";

    html +=
        "<p><b>MAC:</b> " +
        ETH.macAddress() +
        "</p>";

    html +=
        "<p><b>TCP Port:</b> " +
        String(tcpPort) +
        "</p>";

    html += "</div>";

    // =================================================
    // SERIAL CONFIG
    // =================================================

    html += "<div class='card'>";

    html += "<h2>Serial Port</h2>";

    html +=
        "<form action='/save' method='POST'>";

    html +=
        "<input type='hidden' name='csrf' value='" +
        activeSessionToken + "'>";

    html +=
        "<label>Baud Rate</label>";

    html += "<select name='baud'>";

    html +=
        "<option value='9600'>9600</option>";

    html +=
        "<option value='19200'>19200</option>";

    html +=
        "<option value='38400'>38400</option>";

    html +=
        "<option value='57600'>57600</option>";

    html +=
        "<option value='115200'>115200</option>";

    html += "</select>";

    html +=
        "<label>Parity</label>";

    html +=
        "<select name='parity'>";

    html +=
        "<option value='N'>None</option>";

    html +=
        "<option value='E'>Even</option>";

    html +=
        "<option value='O'>Odd</option>";

    html += "</select>";

    html +=
        "<label>Stop Bits</label>";

    html +=
        "<select name='stop'>";

    html +=
        "<option value='1'>1</option>";

    html +=
        "<option value='2'>2</option>";

    html += "</select>";

    html +=
        "<label>TCP Port</label>";

    html +=
        "<input type='number' "
        "name='tcpport' "
        "value='" +
        String(tcpPort) +
        "'>";

    html +=
        "<button type='submit'>"
        "Save Settings"
        "</button>";

    html += "</form>";

    html += "</div>";

    // =================================================
    // DATA MONITOR
    // =================================================

    html += "<div class='card'>";

    html += "<h2>Data Monitor</h2>";

    html +=
        "<p><b>RS485 RX</b></p>";

    html +=
        "<pre>" +
        lastRS485RX +
        "</pre>";

    html +=
        "<p><b>RS485 TX</b></p>";

    html +=
        "<pre>" +
        lastRS485TX +
        "</pre>";

    html +=
        "<p><b>TCP RX</b></p>";

    html +=
        "<pre>" +
        lastTCPRX +
        "</pre>";

    html +=
        "<p><b>TCP TX</b></p>";

    html +=
        "<pre>" +
        lastTCPTX +
        "</pre>";

    html +=
        "<p><b>RS485 RX Bytes:</b> " +
        String(rs485RxBytes) +
        "</p>";

    html +=
        "<p><b>RS485 TX Bytes:</b> " +
        String(rs485TxBytes) +
        "</p>";

    html +=
        "<p><b>TCP RX Bytes:</b> " +
        String(tcpRxBytes) +
        "</p>";

    html +=
        "<p><b>TCP TX Bytes:</b> " +
        String(tcpTxBytes) +
        "</p>";

    html += "</div>";

    // =================================================
    // NETWORK
    // =================================================

    html += "<div class='card'>";

    html +=
        "<h2>Network Configuration</h2>";

    html +=
        "<p><b>IP:</b> " +
        ETH.localIP().toString() +
        "</p>";

    html +=
        "<p><b>Subnet:</b> " +
        ETH.subnetMask().toString() +
        "</p>";

    html +=
        "<p><b>Gateway:</b> " +
        ETH.gatewayIP().toString() +
        "</p>";

    html += "</div>";

    // =================================================
    // SYSTEM
    // =================================================

    html += "<div class='card'>";

    html += "<h2>System</h2>";

    html +=
        "<form action='/reboot' method='POST'>"
        "<input type='hidden' name='csrf' value='" +
        activeSessionToken + "'>";

    html +=
        "<button>Reboot Device</button>";

    html += "</form>";

    html += "</div>";

    html += "</div>";

    html += "</body>";

    html += "</html>";

    return html;
}

// =====================================================
//                    WEB ROOT
// =====================================================

void redirectTo(const char *path)
{
    server.sendHeader("Location", path, true);
    server.send(303, "text/plain", "");
}

String createSessionToken()
{
    char token[33];
    snprintf(
        token,
        sizeof(token),
        "%08lx%08lx%08lx%08lx",
        (unsigned long)esp_random(),
        (unsigned long)esp_random(),
        (unsigned long)esp_random(),
        (unsigned long)esp_random()
    );
    return String(token);
}

bool requestHasValidSession(bool requireCsrf)
{
    if (activeSessionToken.length() == 0)
        return false;

    if ((unsigned long)(millis() - sessionLastActivity) > SESSION_TIMEOUT_MS)
    {
        activeSessionToken = "";
        return false;
    }

    String cookie = server.header("Cookie");
    String expected = "ESPSESSION=" + activeSessionToken;
    int cookieAt = cookie.indexOf(expected);
    if (cookieAt < 0)
        return false;

    int valueEnd = cookie.indexOf(';', cookieAt);
    if (valueEnd < 0)
        valueEnd = cookie.length();
    if (cookie.substring(cookieAt, valueEnd) != expected)
        return false;

    if (requireCsrf && server.arg("csrf") != activeSessionToken)
        return false;

    sessionLastActivity = millis();
    return true;
}

void sendLoginPage(const char *message = "")
{
    String html = R"HTML(<!doctype html><html><head>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Protocol Converter Login</title>
<style>
body{font-family:Arial,sans-serif;background:#f2f4f7;margin:0;display:grid;min-height:100vh;place-items:center}
.card{background:white;padding:28px;border-radius:10px;box-shadow:0 2px 12px #ccd2da;width:min(360px,calc(100% - 56px))}
h1{font-size:22px;margin:0 0 8px;color:#12304a}p{color:#596579}label{display:block;margin-top:14px}
input{box-sizing:border-box;width:100%;padding:11px;margin-top:6px;border:1px solid #bcc6d1;border-radius:5px}
button{margin-top:20px;width:100%;padding:12px;background:#0879c9;color:white;border:0;border-radius:5px;font-size:16px}
.error{color:#b42318;min-height:20px}
</style></head><body><main class='card'>
<h1>Protocol Converter</h1><p>Sign in to view status and change settings.</p>
<div class='error'>)HTML";
    html += message;
    html += R"HTML(</div><form action='/login' method='POST'>
<label>Username<input name='username' autocomplete='username' required></label>
<label>Password<input type='password' name='password' autocomplete='current-password' required></label>
<button type='submit'>Log in</button></form></main></body></html>)HTML";
    server.send(200, "text/html", html);
}

void handleLogin()
{
    if (loginBlockedUntil != 0 && (long)(millis() - loginBlockedUntil) < 0)
    {
        server.send(429, "text/plain", "Too many failed logins. Wait and try again.");
        return;
    }

    const bool usernameOK = server.arg("username") == WEB_USERNAME;
    const bool passwordOK = server.arg("password") == WEB_PASSWORD;
    if (!usernameOK || !passwordOK)
    {
        failedLoginAttempts++;
        if (failedLoginAttempts >= 5)
        {
            failedLoginAttempts = 0;
            loginBlockedUntil = millis() + 30000UL;
        }
        sendLoginPage("Incorrect username or password.");
        return;
    }

    failedLoginAttempts = 0;
    loginBlockedUntil = 0;
    activeSessionToken = createSessionToken();
    sessionLastActivity = millis();
    server.sendHeader(
        "Set-Cookie",
        "ESPSESSION=" + activeSessionToken + "; Path=/; HttpOnly; SameSite=Strict",
        true
    );
    redirectTo("/");
}

void handleLogout()
{
    if (!requestHasValidSession(true))
    {
        redirectTo("/");
        return;
    }

    activeSessionToken = "";
    server.sendHeader(
        "Set-Cookie",
        "ESPSESSION=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0",
        true
    );
    redirectTo("/");
}

void handleRoot()
{
    if (!requestHasValidSession(false))
    {
        sendLoginPage();
        return;
    }

    server.send(
        200,
        "text/html",
        makePage()
    );
}

// =====================================================
//                    SAVE SETTINGS
// =====================================================

void handleSave()
{
    if (!requestHasValidSession(true))
    {
        redirectTo("/");
        return;
    }

    if (server.hasArg("baud"))
    {
        baudRate =
            server.arg("baud").toInt();
    }

    if (server.hasArg("parity"))
    {
        String p =
            server.arg("parity");

        if (p.length() > 0)
            parityMode = p[0];
    }

    if (server.hasArg("stop"))
    {
        stopBits =
            server.arg("stop").toInt();
    }

    if (server.hasArg("tcpport"))
    {
        uint16_t newPort =
            server.arg("tcpport").toInt();

        if (newPort >= 1 &&
            newPort <= 65535)
        {
            tcpPort = newPort;
        }
    }

    // -------------------------------------------------
    // SAVE TO NVS
    // -------------------------------------------------

    prefs.begin(
        "config",
        false
    );

    prefs.putUInt(
        "baud",
        baudRate
    );

    prefs.putChar(
        "parity",
        parityMode
    );

    prefs.putUChar(
        "stop",
        stopBits
    );

    prefs.putUShort(
        "tcp",
        tcpPort
    );

    prefs.end();

    // Restart RS485 with new settings
    startRS485();

    // Apply the saved TCP port immediately and disconnect any old client.
    tcpClient.stop();
    tcpServer.end();
    tcpServer.begin(tcpPort);

    server.send(
        200,
        "text/html",
        "<html>"
        "<body>"
        "<h2>Settings Saved</h2>"
        "<p>RS485 settings updated.</p>"
        "<p>Restarting TCP server...</p>"
        "<a href='/'>Back</a>"
        "</body>"
        "</html>"
    );
}

// =====================================================
//                    REBOOT
// =====================================================

void handleReboot()
{
    if (!requestHasValidSession(true))
    {
        redirectTo("/");
        return;
    }

    server.send(
        200,
        "text/html",
        "<html>"
        "<body>"
        "<h2>Rebooting...</h2>"
        "</body>"
        "</html>"
    );

    delay(1000);

    ESP.restart();
}

// =====================================================
//                    START WEB SERVER
// =====================================================

void startWebServer()
{
    server.collectHeaders(REQUEST_HEADERS, 1);

    server.on(
        "/",
        HTTP_GET,
        handleRoot
    );

    server.on("/login", HTTP_POST, handleLogin);
    server.on("/logout", HTTP_POST, handleLogout);

    server.on(
        "/save",
        HTTP_POST,
        handleSave
    );

    server.on(
        "/reboot",
        HTTP_POST,
        handleReboot
    );

    server.begin();

    Serial.println();

    Serial.println(
        "================================"
    );

    Serial.println(
        "WEB SERVER STARTED"
    );

    Serial.println(
        "================================"
    );

    Serial.print(
        "Open: http://"
    );

    Serial.println(
        ETH.localIP()
    );
}

// =====================================================
//                    SETUP
// =====================================================

void setup()
{
    // IMPORTANT:
    // Set Arduino Serial Monitor to 115200

    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println();

    Serial.println(
        "========================================"
    );

    Serial.println(
        " ESP32-S3 PROTOCOL CONVERTER"
    );

    Serial.println(
        "========================================"
    );

    // =================================================
    // RS485 DIRECTION
    // =================================================

    pinMode(
        RS485_DE_PIN,
        OUTPUT
    );

    digitalWrite(
        RS485_DE_PIN,
        LOW
    );

    // =================================================
    // LOAD SAVED SETTINGS
    // =================================================

    loadSettings();

    // =================================================
    // START RS485
    // =================================================

    startRS485();

    // =================================================
    // START ETHERNET
    // =================================================

    if (!startEthernet())
    {
        Serial.println();
        Serial.println(
            "W5500 initialization failed."
        );

        Serial.println(
            "Check W5500 wiring."
        );

        return;
    }

    // =================================================
    // WAIT FOR ETHERNET
    // =================================================

    Serial.println();

    Serial.println(
        "Waiting for Ethernet..."
    );

    unsigned long startTime =
        millis();

    while (
        !ETH.linkUp() &&
        (millis() - startTime < 10000)
    )
    {
        delay(100);

        Serial.print(".");
    }

    Serial.println();

    // =================================================
    // CHECK ETHERNET
    // =================================================

    updateEthernetStatus();

    if (!ETH.linkUp())
    {
        Serial.println();

        Serial.println(
            "WARNING: Ethernet link not detected."
        );

        Serial.println(
            "Check Ethernet cable/W5500."
        );
        Serial.println(
            "Starting the servers anyway; they will be available when link comes up."
        );
    }

    // =================================================
    // WAIT FOR IP
    // =================================================

    Serial.println(
        "Waiting for IP..."
    );

    startTime = millis();

    while (
        ETH.localIP() == IPAddress(0, 0, 0, 0) &&
        (millis() - startTime < 5000)
    )
    {
        delay(100);

        Serial.print(".");
    }

    Serial.println();

    // =================================================
    // DISPLAY NETWORK
    // =================================================

    Serial.println();

    Serial.println(
        "================================"
    );

    Serial.println(
        "ETHERNET STATUS"
    );

    Serial.println(
        "================================"
    );

    Serial.print(
        "IP Address : "
    );

    Serial.println(
        ETH.localIP()
    );

    Serial.print(
        "Subnet     : "
    );

    Serial.println(
        ETH.subnetMask()
    );

    Serial.print(
        "Gateway    : "
    );

    Serial.println(
        ETH.gatewayIP()
    );

    Serial.print(
        "MAC        : "
    );

    Serial.println(
        ETH.macAddress()
    );

    // =================================================
    // START WEB SERVER
    // =================================================

    startWebServer();

    // =================================================
    // START TCP SERVER
    // =================================================

    tcpServer.begin(tcpPort);

    Serial.println();

    Serial.println(
        "================================"
    );

    Serial.println(
        "TCP SERVER STARTED"
    );

    Serial.println(
        "================================"
    );

    Serial.print(
        "TCP Port: "
    );

    Serial.println(
        tcpPort
    );

    Serial.println();

    Serial.println(
        "Protocol Converter Ready."
    );
}

// =====================================================
//                    LOOP
// =====================================================

void loop()
{
    // -------------------------------------------------
    // UPDATE ETHERNET STATUS
    // -------------------------------------------------

    updateEthernetStatus();

    // -------------------------------------------------
    // WEB SERVER
    // -------------------------------------------------

    server.handleClient();

    // -------------------------------------------------
    // TCP
    // -------------------------------------------------

    if (ethConnected)
    {
        checkTCPClient();

        tcpToRS485();

        rs485ToTCP();
    }

    delay(1);
}
