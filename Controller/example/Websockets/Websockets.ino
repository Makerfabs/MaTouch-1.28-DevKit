/*
Library version:
Arduino IDE 2.3.6
esp32 V3.2.0
GFX Library for Arduino v1.6.0
DHT sensor library v1.4.6
Adafruit Unified Sensor v1.1.14
WebSockets v2.7.0

Tools:
USB CDC On Boot: Enabled
*/
#include <Arduino_GFX_Library.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

const char* ssid = "Makerfabs";
const char* password = "20160704";

WiFiServer server(80);
WebSocketsServer webSocket(81);

#define DHTPIN 16
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

float humi,temp;

#define TFT_BLK 45
#define TFT_RES 21
#define TFT_CS 1
#define TFT_MOSI 2
#define TFT_MISO -1
#define TFT_SCLK 42
#define TFT_DC 46

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0 /* rotation */, true /* IPS */);

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Websocket Demo Real-time T&H</title>
  <style>
    body { font-family: Arial; text-align: center; margin-top: 50px; background-color: #f0f0f0; } /* Body styles: font, centering, top margin, and background color */
    .container { max-width: 500px; margin: 0 auto; padding: 20px; background-color: white; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); } /* Main container styles: max width, centering, padding, background, border radius, and shadow */
    h1 { color: #333; }
    .data-container { display: flex; justify-content: space-around; margin-top: 30px; } /* Data container styles: flex layout, evenly spaced, and top margin */
    .data-box { padding: 15px; border-radius: 8px; width: 45%; } /* Data box styles: padding, border radius, and width */
    .temp-box { background-color: #e3f2fd; }
    .humid-box { background-color: #e1f5fe; }
    .value { font-size: 36px; font-weight: bold; margin: 10px 0; } /* Value styles: font size, bold, and vertical margins */
    .temp-value { color: #f44336; }
    .humid-value { color: #2196f3; }
    .label { font-size: 18px; color: #555; } /* Label styles: font size and color */
    .icon { font-size: 24px; margin-right: 5px; } /* Icon styles: font size and right margin */
  </style>
</head>
<body>
  <div class="container"> <!-- Main container div -->
    <h1>Websocket Demo Real-time T&H</h1> <!-- Page title -->
    <div class="data-container"> <!-- Start of data container -->
      <div class="data-box temp-box"> <!-- Temperature data box -->
        <div class="label"><span class="icon">🌡️</span>Temperature</div> <!-- Temperature label and icon -->
        <div id="temp" class="value temp-value">--°C</div> <!-- Temperature value display area, initial placeholder -->
      </div>
      <div class="data-box humid-box"> <!-- Humidity data box -->
        <div class="label"><span class="icon">💧</span>Humidity</div> <!-- Humidity label and icon -->
        <div id="humid" class="value humid-value">--%</div> <!-- Humidity value display area, initial placeholder -->
      </div>
    </div>
  </div>
  <script>
    var ws = new WebSocket("ws://" + location.hostname + ":81/"); // Create a WebSocket connection to the ESP32 on port 81
    ws.onmessage = function(event) { // Define the WebSocket message handler
      var data = event.data.split(','); // Split the received data by comma
      if(data.length >= 2) { // Check whether the data contains at least two values (temperature and humidity)
        document.getElementById("temp").innerText = data[0] + "°C"; // Update temperature display
        document.getElementById("humid").innerText = data[1] + "%"; // Update humidity display
      }
    };
  </script>
</body>
</html>
)rawliteral";

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected to WiFi, IP address: " + WiFi.localIP().toString());

  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  Serial.println("start");
  dht.begin();
  gfx->begin();
  gfx->fillScreen(WHITE);
  gfx->setTextSize(2);
  gfx->setTextColor(BLACK);
  gfx->setCursor(45, 50);
  gfx->println(F("Websocket Demo"));
  gfx->setCursor(20, 175);
  gfx->print(F("IP:"));
  gfx->print(WiFi.localIP().toString());
  gfx->setCursor(20, 95);
  gfx->println(F("Temperature: "));
  gfx->setCursor(20, 135);
  gfx->println(F("Humidity: "));

  server.begin();
  webSocket.begin();
}

void loop()
{
  // 处理 HTTP 请求，返回 HTML
  WiFiClient client = server.available();
  if (client)
  {
    String req = client.readStringUntil('\r');
    client.flush();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.print(htmlPage);
    delay(1);
  }

  webSocket.loop();

  humi = dht.readHumidity();
  temp = dht.readTemperature();

  static unsigned long lastSend = 0;
  if (millis() - lastSend > 1000)
  {
    String dataString = String(temp) + "," + String(humi);
    webSocket.broadcastTXT(dataString.c_str());

    gfx->fillRect(165, 90, 70, 30, WHITE);
    gfx->setCursor(170, 95);
    gfx->print(temp);
    gfx->fillRect(130, 130, 70, 30, WHITE);
    gfx->setCursor(135, 135);
    gfx->print(humi);
    lastSend = millis();
  }
}
